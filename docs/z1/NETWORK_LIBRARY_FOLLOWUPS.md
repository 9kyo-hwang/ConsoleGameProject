# Z1 네트워크 라이브러리 확장 검토 메모

## 문서 역할

이 문서는 현재 Z1Server에 구현하지 않았거나 보류한 IOCP·Session·송신 구조 후보를 나중에 재검토하기 위한 메모다. [멀티플레이 설계](MULTIPLAYER_DESIGN.md)의 확정 계약이나 [멀티플레이 현황](MULTIPLAYER_STATUS.md)의 다음 구현 단위를 대체하지 않는다. 구현을 결정하면 해당 문서들에 결과와 검증 방법을 반영한다.

비교 대상으로 삼은 Rookiss ServerCore는 학습용 IOCP 서버 구조다. 여기서의 클래스 모양을 그대로 옮기는 것이 목표가 아니라, Z1의 규모와 현재 책임 경계에 맞는 원칙만 선택한다.

## 현재 기준선

Z1Server는 다음의 작은 구조를 유지한다.

- `CompletionPort`가 completion port HANDLE을 RAII로 소유하고, 생성·association·dequeue·shutdown post를 감싼다.
- accept thread는 새 socket만 queue에 넣는다. IO thread가 Session 등록, IOCP completion 처리, packet dispatch와 20Hz simulation을 소유한다.
- `Session`은 고정 recv buffer와 `PacketFramer`, packet 단위의 send queue, partial send offset을 가진다.
- Snapshot은 수신 Player의 관심 Room에 따라 만들어지므로 현재는 Session마다 직렬화한다.

`PacketFramer`가 TCP 수신 누적, packet 경계 복원과 잘못된 크기 거부를 담당하므로, Rookiss의 sliding `RecvBuffer`를 별도로 도입하지 않는다. 송신도 현재는 한 번에 하나의 완성 packet을 보내며 partial send를 명시적으로 처리한다.

## 보류한 클라이언트 논블로킹 연결

### 확인된 현재 동작

2026-09-04 Local Play와 Multiplayer의 Title 진입 경로를 분리하는 작업 중, Z1Server를 실행하지 않은 상태에서 Multiplayer를 선택하면 화면과 입력이 약 2초간 멈추는 현상을 확인했다.

현재 `NetworkClient::Start()`는 다음 순서로 실행된다.

1. `Socket::CreateTcp()`로 기본 blocking socket을 만든다.
2. main thread에서 `Socket::Connect()`를 호출한다.
3. 연결이 성공한 뒤 `SetNoDelay(true)`와 `SetNonBlocking(true)`를 호출한다.
4. `C2S_Enter`를 queue에 넣고 network thread를 시작한다.

따라서 최초 `connect()`만 blocking으로 실행되며, 이 호출이 반환할 때까지 main thread의 Tick, 입력 처리와 렌더링도 함께 멈춘다. Title의 3초 타이머도 `NetworkClient::Start()`가 반환된 뒤 시작하므로 현재 제한 시간에는 blocking connect 시간이 포함되지 않는다.

닫힌 localhost 포트는 일반적으로 빠르게 연결 거부를 반환할 수 있지만, 실제 실패 시간은 방화벽·보안 필터·TCP stack과 패킷 처리 방식에 따라 달라질 수 있다. 이번에 관찰한 약 2초를 Windows의 고정된 SYN 재전송 시간이나 특정 방화벽 정책으로 단정하지 않는다. 정확한 원인을 구분하려면 반환된 WinSock 오류와 packet trace가 필요하다.

### WinSock 논블로킹 connect 계약

`SetNonBlocking(true)`를 `Connect()`보다 먼저 호출하면 `connect()`도 논블로킹으로 동작한다.

- `Connect()`가 즉시 성공하면 TCP 연결이 완료된 상태다.
- `SOCKET_ERROR`와 `WSAEWOULDBLOCK`은 실패가 아니라 연결 시도가 진행 중이라는 뜻이다.
- 진행 중인 `connect()`의 성공은 `select()`의 `writefds`, 실패는 `exceptfds`로 확인한다.
- 완료 알림 뒤에는 `getsockopt(SOL_SOCKET, SO_ERROR)`로 socket에 기록된 최종 오류를 확인한다. 기존 `Socket::GetLastError()`는 직전에 실패한 API의 `WSAGetLastError()` 값을 보관하므로 이 조회를 대신하지 않는다.
- 완료를 확인하려고 같은 socket에 `connect()`를 반복 호출하지 않는다.
- 연결 진행 중의 `setsockopt()`는 지원되지 않으므로 `SetNoDelay(true)`는 연결 완료 뒤 호출한다.

참고: [Microsoft connect](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-connect), [Microsoft select](https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-select), [Microsoft getsockopt](https://learn.microsoft.com/en-us/windows/win32/api/winsock/nf-winsock-getsockopt)

### 검토한 두 접근

#### NetworkLoop에서 blocking Connect 호출

`Start()`가 thread만 시작하고 `NetworkLoop()`의 앞부분에서 blocking `Connect()`를 실행하면 main thread 정지는 피할 수 있다. 하지만 network thread가 `connect()` 안에서 기다리는 동안 `_stopRequested`를 확인하지 못하고, `Stop()`의 `join()`도 연결 시도가 반환될 때까지 기다릴 수 있다. 따라서 TCP 연결과 Enter 응답을 합쳐 최대 3초로 제한하려는 요구를 보장하지 못한다.

UI 정지만 제거하고 OS connect timeout을 그대로 허용하는 임시 구현으로는 가능하지만, 현재 구조의 최종안으로 채택하지 않는다.

#### 기존 select loop에 논블로킹 연결 상태 추가

명시적인 연결 제한 시간과 취소 가능한 종료가 필요해지면 이 방식을 우선한다.

```text
Start
├─ socket 생성과 nonblocking 설정
├─ Connect
│  ├─ 즉시 성공       -> 연결 완료 처리
│  ├─ WSAEWOULDBLOCK -> Connecting
│  └─ 그 외 오류      -> Disconnected
└─ network thread 시작

NetworkLoop
├─ Connecting -> select(writefds, exceptfds) -> SO_ERROR 확인
└─ Connected  -> 기존 select(readfds, 필요할 때 writefds) 송수신
```

이 변경에는 단순한 호출 순서 이동 외에 다음 작업이 함께 필요하다.

- 기존 atomic `_connected`를 `Disconnected`, `Connecting`, `Connected` 상태로 대체한다.
- `Start()`의 성공은 TCP 연결 완료가 아니라 연결 시도의 정상 시작을 뜻하도록 계약을 바꾼다.
- Title은 `Connecting`을 실패로 판단하지 않고, `Disconnected` 전이 또는 전체 제한 시간 만료를 기다린다.
- `SetNoDelay(true)`와 `C2S_Enter` 생성·queue는 TCP 연결 완료 뒤 공통 완료 경로에서 실행한다.
- 연결 중에는 정상 `recv`/`send`를 하지 않고 `writefds`와 `exceptfds`만 감시한다.
- `Socket`에는 `SO_ERROR` 조회를 위한 작은 API를 추가한다. 비동기 오류 코드를 main thread에 상세히 표시하지 않는다면 별도의 atomic last-error 값은 필요 없다.
- 현재 `NetworkClient::Stop()`은 thread join 뒤 socket, main→network 전송 queue, pending 전송 packet, network→main 수신 queue, framer, partial-send offset과 input sequence를 초기화한다. 논블로킹 연결을 도입하더라도 이 세션 정리 계약을 유지한다.

### 현재 결정과 재검토 조건

이번 모드 분리 작업에서는 변경 범위가 연결 상태, network loop, Enter 전송 시점과 종료 수명까지 커지므로 논블로킹 connect를 구현하지 않는다. 현재 blocking connect를 알려진 제한으로 유지하고 Local/Multiplayer 진입 분리를 먼저 완료한다.

다음 조건 중 하나가 필요해지면 이 결정을 재검토한다.

- 서버 미실행 또는 응답 없는 endpoint에서 Title의 입력·렌더링 정지가 실제 사용 문제로 남는다.
- localhost 밖의 endpoint를 지원한다.
- TCP 연결과 `S2C_Enter` 응답을 합친 명시적인 최대 대기 시간을 보장해야 한다.
- 사용자가 연결 시도를 취소할 수 있어야 한다.

구현을 재개할 때의 최소 검증은 다음과 같다.

- 서버 미실행 상태에서도 Title의 Tick, 렌더링과 입력이 멈추지 않는다.
- 서버 실행 상태에서 connect, `C2S_Enter`, `S2C_Enter` 뒤 Network Overworld로 진입한다.
- 연결 실패와 Enter 무응답을 구분하지 않더라도 정해진 제한 시간 안에 socket과 thread를 정리하고 Title 메뉴로 돌아간다.
- 실패 후 Multiplayer 재시도에서 이전 thread, packet, playerId와 Snapshot 상태가 남지 않는다.

## 보류한 Scatter-Gather 송신

Rookiss의 `SendBuffer`와 다중 `WSABUF` 송신은 여러 완성 packet을 하나의 `WSASend`에 등록하는 구조다. `IocpEvent`가 `shared_ptr<SendBuffer>` 목록을 보유해 완료 전 packet 메모리 수명을 보장하고, 같은 packet을 여러 Session이 공유할 수도 있다.

이는 현재 Z1 Snapshot에 바로 맞지 않는다.

- Snapshot은 최신 상태가 중요하다. 밀린 여러 Snapshot을 함께 전송하면 이미 오래된 상태까지 전달해 지연을 늘릴 수 있다.
- 관심 Room이 다르면 payload도 달라 일반적인 전역 broadcast packet 공유가 불가능하다.
- Z1의 `deque<vector<Byte>>`와 offset은 TCP partial send를 직접 처리한다. 이를 일반화하기 전에 실제 송신 병목을 확인해야 한다.

향후 송신 최적화가 필요하면 적용 순서는 다음과 같다.

1. 미전송 Snapshot을 최신 Snapshot으로 교체하는 coalescing을 먼저 검토한다.
2. 동일 `RoomCoordinate`와 server tick의 Snapshot이 수신자별 필드를 갖지 않는 것이 확인되면, Room별로 한 번 직렬화한 `shared_ptr<const vector<Byte>>`를 해당 Room Session이 공유한다.
3. 그 뒤에도 작은 신뢰성 이벤트 packet이 많이 쌓여 syscall·복사 비용이 측정되면 `SendBuffer`와 multi-`WSABUF`를 검토한다.

TCP는 `WSABUF` 경계를 수신자에게 보존하지 않는다. Scatter-Gather를 도입해도 Z1Shared의 header와 `PacketFramer`는 그대로 필요하다.

## 차용 후보

### 1. Session 종료 상태와 outstanding I/O 수명 관리 (현재 적용)

가장 우선순위가 높은 후보다. socket을 닫는 시점과 Session 객체를 파괴해도 되는 시점은 다르다. 닫힌 socket의 overlapped recv/send completion은 나중에 도착할 수 있으므로, completion key가 가리키는 Session을 즉시 제거하면 안 된다.

Rookiss는 I/O event가 owner `shared_ptr`를 보유해 이 수명을 보장한다. Z1은 `unique_ptr<Session>` registry와 stable `Session*` completion key를 유지하면서 같은 수명을 다음처럼 보장한다.

- `_closing` flag로 종료 전이를 idempotent하게 만들고, Player 제거와 socket close는 최초 전이에서 한 번만 실행한다.
- Snapshot·CombatEvent·EnemyPathDebug 대상과 새 I/O 등록을 closing이 아닌 entered Session으로 제한한다.
- `_recvPending`과 `_sendPending`을 추적하고, IOLoop이 Recv/Send `OVERLAPPED` completion을 성공·실패·취소 여부와 관계없이 acknowledge한다.
- IOLoop 반복문 시작의 deferred sweep에서 `closing && no pending I/O` Session만 registry에서 제거한다.

registry에서 장기적으로 제거하려면 두 선택지가 있다.

| 선택지 | 장점 | 비용 |
| --- | --- | --- |
| 현재 `unique_ptr` registry를 유지하고 pending operation 수를 추적 | 현재 구조를 가장 작게 확장 | cancellation completion과 deferred erase 규칙을 명확히 해야 함 |
| operation context가 `shared_ptr<Session>`을 보유 | Rookiss와 같은 명확한 완료 전 수명 보장 | operation 구조와 Session 소유 모델이 커짐 |

현재는 첫 번째 선택지를 적용한다. raw `Session*` completion key를 유지한 채 즉시 erase하는 방식은 허용하지 않는다. `Server::Stop()`에서 모든 Session을 닫고 IOCP completion을 drain하는 전체 종료 barrier와 더 세분화된 operation context는 아직 후속 검토 대상이다.

### 2. 접속·큐 상한과 backpressure

Rookiss `Service`의 최대 Session 수 관리에서 가져올 수 있는 원칙이다. Z1의 Session send queue는 이미 상한을 두고 느린 수신자를 정리한다. 같은 방식으로 다음 경계를 필요해질 때 추가한다.

- 동시 Session 최대 수
- accept thread에서 IO thread로 넘기는 `_acceptedSockets` 최대 길이
- 한 completion에서 처리하는 packet 수 또는 simulation 전에 소비할 input 작업량 상한

이는 특정 자료구조를 도입하는 작업이 아니라, 느린 client나 대량 접속이 simulation Tick을 밀지 않도록 하는 방어다. 실제 다중 client 시험에서 queue 길이 또는 Tick 지연이 관찰될 때 추가한다.

### 3. 완료 dispatch의 Session 위임

Rookiss의 `IocpObject::Dispatch`는 completion을 소유 객체에 전달해 IOCP core가 Session·Listener의 세부 operation을 몰라도 되게 한다.

현재 Z1은 recv/send 두 operation뿐이고 Server가 overlapped 주소를 비교하는 흐름이 명확하다. 따라서 `IocpObject` 상속 계층이나 범용 event type을 지금 도입하지 않는다.

accept, timer, 여러 송신 종류처럼 분기가 늘어나 Server의 IO loop가 실제로 복잡해지면 다음 정도의 작은 위임을 검토한다.

```cpp
enum class SessionCompletionResult
{
    ProcessPackets,
    SendCompleted,
    PeerClosed,
    Failed,
    UnknownOperation,
};

SessionCompletionResult Session::HandleCompletion(
    OVERLAPPED* overlapped,
    bool succeeded,
    DWORD bytesTransferred);
```

이 경우에도 `CompletionPort`는 Windows completion dequeue만 담당하고, Session은 transport 상태 갱신, Server는 packet의 게임 규칙 처리를 담당한다.

### 4. Room별 불변 packet 공유

Rookiss의 Room broadcast는 하나의 불변 `SendBuffer`를 여러 Session이 공유한다. Z1에서도 같은 Room의 Snapshot이 수신자별 차이 없이 동일하다는 것이 확인되면 적용할 수 있다.

```text
Room snapshot 한 번 직렬화
        |
shared_ptr<const Packet>
  |       |       |
Session A Session B Session C
```

이것은 Scatter-Gather와 독립된 최적화다. 먼저 packet 공유만 도입해도 Session별 serialization·payload allocation을 줄일 수 있다. local player 전용 필드나 수신자별 관심 규칙이 생기면 공유 key와 packet 구성 방식을 다시 확인한다.

## 현재 채택하지 않는 구조

| Rookiss 구조 | 현재 Z1에서 보류하는 이유 | 재검토 조건 |
| --- | --- | --- |
| 범용 `Service` / `SessionFactory` | Z1Server 하나만 대상으로 하며 Server 시작 흐름이 작음 | 서버·클라이언트 service를 같은 기반으로 여러 개 운영할 때 |
| `Listener` + `AcceptEx` 다중 pre-post | blocking accept thread와 socket queue가 현재 요구에 충분함 | 대량 동시 접속 또는 accept가 실제 병목일 때 |
| 범용 `IocpObject` 상속 구조 | 현재 completion 대상이 사실상 Session뿐 | Listener, timer 등 서로 다른 completion owner가 늘어날 때 |
| `RecvBuffer` | `PacketFramer`가 Z1 protocol의 필요한 수신 누적을 이미 담당 | framing과 buffer compaction이 성능 병목으로 측정될 때 |
| `SendBuffer` pool과 multi-`WSABUF` | Snapshot stale-state 문제와 partial-send 복잡도가 이득보다 큼 | packet 복사·syscall 비용이 측정되고 coalescing/공유 뒤에도 부족할 때 |
| 전역 thread manager / worker pool | simulation의 단일 thread 소유권이 현재 더 안전하고 단순함 | Tick·IO 처리량 측정으로 분리가 필요한 경우 |

## 재검토 순서

1. Enemy 이동·전투 같은 현재 게임 simulation 작업을 완료한다.
2. 여러 실제 Z1 client와 dummy client로 Tick 시간, send queue high-water mark, packet 크기, 접속·종료 반복을 측정한다.
3. 종료 Session이 Player 제거·Snapshot 제외·I/O completion drain을 정확히 한 번씩 처리하는지 먼저 보완한다.
4. 관찰된 병목 하나에만 대응하는 항목을 선택한다. 추정만으로 Service, worker pool, Scatter-Gather를 함께 도입하지 않는다.
