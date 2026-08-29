# Z1 멀티플레이 개발 현황

## 문서 목적

이 문서는 Z1 멀티플레이 구현의 현재 상태, 검증 결과, 알려진 제약과 다음 작업을 기록한다. 확정된 구조와 계약, 단계별 완료 조건은 [Z1_MULTIPLAYER_IOCP_PLAN.md](Z1_MULTIPLAYER_IOCP_PLAN.md)를 기준으로 한다.

- 설계와 계약이 바뀌면 아키텍처 문서를 먼저 갱신하고, 이 문서에는 변경 사실과 이유를 기록한다.
- 구현 여부, 실제 테스트 결과와 다음 재개 지점은 이 문서에서만 관리한다.
- 이 문서와 코드가 다르면 코드를 우선 확인하고, 같은 작업에서 현황을 갱신한다.

## 현재 구현 진행 상황 (2026-08-29)

이 절은 목표 구조가 아니라 현재 작업 트리와 실제 확인 결과를 기준으로 한다. 다음 에이전트나 모델은 이 절과 코드의 차이가 있으면 코드를 우선 확인하고, 작업을 이어갈 때 이 절을 갱신한다.

### 완료·확인된 범위

- Z1, SokobanGame, ShootingGame의 기존 싱글플레이 상태는 유지되고, 사용자가 Debug|x64 빌드·실행 정상임을 확인했다.
- 설계상 `SocketAPI` 역할을 담당하는 실제 Visual Studio 프로젝트 `Sockets`를 추가했다. `Sockets.dll`/`Sockets.lib`를 만들고 공개 헤더와 DLL·import library를 `Includes/Sockets`, `Libraries/Sockets/<Configuration>`으로 staging하는 빌드 이벤트를 설정했다.
- `Sockets`에는 `Net::Runtime`, `Net::Endpoint`, 이동 전용 `Net::Socket`이 있다.
  - `Runtime`: 프로세스의 `WSAStartup`/`WSACleanup` 수명 관리
  - `Endpoint`: 공개 API에서 `SOCKADDR_IN`을 숨기고 `Any`, `Loopback`, IPv4 파싱 제공
  - `Socket`: `CreateTcp`, `Bind`, `Listen`, `Accept`, `Connect`, `SetNonBlocking`, `SetNoDelay`, `Send`, `Recv`, `Close`, `ReleaseNativeSocket`와 유효성·이동 의미 제공
- `Sockets`는 필요한 WinSock 링크(`Ws2_32.lib`)를 갖는다. `SetNoDelay`은 내부의 최소 `setsockopt` helper로 제공하며, 일반 `getsockopt`/`setsockopt` 공개 API는 아직 필요하지 않다.
- `Z1Server` 콘솔 프로젝트를 추가하고 `Sockets`와 WinSock에 링크했다. `Server`가 listen socket과 raw IOCP `HANDLE`을 직접 소유하는 최소 구조이며, IOCP를 `SocketAPI` 공개 API로 올리지는 않았다.
- `Z1Shared/Protocol.h`, `Z1Shared/Serialization.h`를 실제 공유 헤더 디렉터리로 추가했다. 고정 길이 정수는 WinSock 함수 없이 명시적인 endian 변환과 `memcpy`로 big-endian wire byte를 만들며, raw struct/`bool` 메모리를 그대로 전송하지 않는다.
- `Server::Start`는 TCP socket 생성 → `Bind` → `Listen` → completion port 생성 순으로 동작하고, blocking `AcceptLoop`와 IOCP `IOLoop`을 각각 스레드로 시작한다.
- 서버를 실행해 `Bind`/`Listen` 후 대기하는 상태를 확인했고, 별도 PowerShell에서 다음 명령으로 접속 경로를 검증했다.

  ```powershell
  Test-NetConnection 127.0.0.1 -Port 7777
  ```

  결과는 `TcpTestSucceeded : True`였고 서버에 `Client Connected!`가 출력됐다. 이후 아래의 payload 왕복·snapshot 검증까지 완료했으므로, 이 명령은 현재도 listen/accept smoke check 용도로만 사용한다.

### 현재 구현된 서버 vertical slice

- `Session`은 복사를 금지하고, accepted `Net::Socket`, recv/send 각각의 `OVERLAPPED`, `WSABUF`, recv 누적 buffer, 완성 packet queue, send queue를 소유한다.
  - recv: 4096바이트 임시 buffer에서 누적 buffer로 옮긴 뒤 `[size:uint16][type:uint16][payload]` framing을 처리한다. 불완전 header/payload는 다음 recv까지 보관하고, 잘못된 전체 크기와 알 수 없는 packet type은 세션 오류로 처리한다.
  - send: Session마다 한 개의 `WSASend`만 pending 상태로 둔다. partial send는 남은 구간으로 다시 등록하고, 완료 전에는 send buffer를 가진 queue 항목을 제거하지 않는다.
  - send queue는 최대 64 packet이다. 20Hz snapshot 때문에 느린 클라이언트의 queue가 무한히 늘어나는 것을 막으며, 상한을 넘긴 `Send`는 실패하고 서버가 해당 세션을 닫는다.
- `AcceptLoop`는 `Net::Socket`을 mutex 보호 `std::deque`에 넣기만 한다. `IOLoop`가 loop 시작 시 queue를 비우고 Session 생성, IOCP 연결, 첫 `WSARecv` 등록, `_sessions` 등록을 모두 수행한다. 따라서 Session registry와 `OverworldSimulation`은 I/O thread 하나만 변경한다.
- `IOLoop`는 completion 처리와 고정 simulation tick을 함께 수행한다. `steady_clock` 기준 50ms(20Hz)마다 `Tick()`을 호출하며, 늦어진 tick은 한 loop에서 최대 5회까지만 catch-up하고 과도하게 밀린 시간은 현재 시각 기준으로 재정렬한다.
- `C2S_Enter`/`S2C_Enter` 왕복을 구현했다.
  - `C2S_Enter` payload: `protocolVersion:uint16`.
  - `S2C_Enter` payload: `protocolVersion:uint16`, 서버 발급 `playerId:uint32`.
  - 첫 Player ID의 전체 응답은 `00 0A 00 65 00 01 00 00 00 01`이며, 전체 크기 10, type 101, version 1, playerId 1을 뜻한다.
- `C2S_Input`을 구현했다. payload는 `sequence:uint32`, `MoveDirection:uint8` (`None/Up/Down/Left/Right`), `actionFlags:uint8`이다. 서버는 세션의 playerId로만 상태를 찾고, 방향/flag 범위와 payload 전체 소비를 검증한다. 오래되었거나 중복된 sequence는 연결을 끊지 않고 무시한다.
- headless `OverworldSimulation`을 추가했다. `CraftEngine`, Z1 Actor, Renderer, Sprite를 링크하지 않으며, 현재는 Player 상태만 소유한다.
  - Player는 서버 월드 좌표 `x/y`, facing, HP 20, dead, 최신 input, 마지막 input sequence를 가진다.
  - 시작 위치는 기존 Overworld의 StartRoom `(7, 7)`과 local tile `(7, 2)`에서 계산한 `(1190, 395)`다.
  - 20Hz에서 Player는 tick당 1 console-cell, 즉 초당 20셀 이동한다. 현재는 전체 Overworld 사각형 경계만 검사하며, blocking tile, Room, Player 간 충돌은 아직 적용하지 않았다.
- `S2C_WorldSnapshot`을 매 서버 tick마다 entered Session 전체에 broadcast한다. 현재 payload는 다음과 같다.

  ```text
  [serverTick:uint32]
  [playerCount:uint16]
  반복 playerCount회:
    [playerId:uint32][x:int32][y:int32]
    [facing:uint8][hp:int32][flags:uint8]
  [enemyCount:uint16 = 0]
  [projectileCount:uint16 = 0]
  ```

  Player 상태는 playerId 순으로 정렬해 snapshot에 기록한다. `serverTick`은 서버 시작 뒤 누적된 simulation tick 번호이며, 현재 클라이언트는 아직 없지만 이후 최신 snapshot 판별·디버깅·행동 sequence의 시간축으로 사용한다.
- `CloseSession`은 entered Session의 Player를 `OverworldSimulation`에서 제거한다. Snapshot broadcast 검증에서 두 번째 Player가 연결된 동안에는 `players=2`, 연결 종료 뒤에는 다음 tick부터 `players=1`이 되는 것을 확인했다.
- `tools/test-z1-enter.ps1`은 PowerShell dummy client다.
  - 기본 실행: `C2S_Enter`/`S2C_Enter` framing과 응답 검증
  - `-SplitSend`: Enter packet을 2바이트와 4바이트로 나누어 header fragmentation 처리 확인
  - `-SplitAt 1..5`: Enter packet의 header 또는 payload를 지정 위치에서 분할
  - `-CoalescedEnterInput`: Enter와 Input을 연속 byte로 한 번에 전송
  - `-SendInput [-InputDirection Up|Down|Left|Right|None] [-HoldMilliseconds N]`: 입력과 정지 입력 전송
  - `-ReadSnapshots N`: Snapshot N개를 파싱해 Player/Enemy/Projectile count와 Player 상태 출력
  - `-InvalidPacketSize 0|3|4097`, `-ProtocolVersion 2`: 잘못된 입력을 서버가 연결 종료로 거부하는지 확인
  - `-RunServerFramingSuite`: 위 서버 framing 검증을 한 번에 실행

### 현재 구현된 Z1 클라이언트 transport

- `Z1`은 `Sockets` DLL/lib와 `Z1Shared` 헤더를 참조하고, 실행 파일 옆에 `Sockets.dll`을 staging한다. Z1 PCH는 `Windows.h`보다 먼저 `WinSock2.h`를 포함해 WinSock 재정의 충돌을 막는다.
- `NetworkClient`는 `Game`의 직접 멤버이며 `Net::Runtime`과 단일 TCP socket을 소유한다. `Game::StartNewGame`은 현재 개발용 loopback `127.0.0.1:7777`에 접속을 한 번 시도한다. 서버가 없으면 접속만 실패하고 기존 싱글플레이 흐름은 계속된다.
- 연결 성공 뒤 `NetworkClient`는 `TCP_NODELAY`와 non-blocking을 설정하고, protocol version만 담은 `C2S_Enter`를 outgoing queue에 넣은 뒤 network thread를 시작한다.
- network thread는 `select`(50ms timeout)로 read/write readiness를 기다린다.
  - 게임 main thread는 직렬화된 packet만 mutex 보호 outgoing queue에 넣는다.
  - network thread는 이를 전용 pending-send queue로 옮기고 partial send offset을 유지해 `Send`한다.
  - 수신 byte는 공용 `PacketFramer`에서 `[size][type][payload]` framing을 거쳐 `S2C_Enter`와 `S2C_WorldSnapshot`으로 역직렬화한다.
  - 역직렬화한 `EnterMessage`/`WorldSnapshot`은 mutex 보호 incoming queue로만 넘긴다. 두 queue의 현재 상한은 각 64 packet/message다.
- `NetworkClient::Stop`은 종료 요청 후 network thread를 join한다. socket close와 `_connected = false` 전환은 network thread가 loop를 빠져나오는 한 곳에서 수행하므로 main thread와 socket handle을 동시에 조작하지 않는다.
- `OverworldLevel::Tick`의 시작에서 main thread가 `Game::PumpNetwork`으로 incoming queue를 비운 뒤 최신 `WorldSnapshot`을 적용한다. local playerId는 기존 local `Player` 표현을 유지하므로 건너뛰고, 나머지 playerId는 `NetworkPlayer`를 생성·갱신·제거한다. 연결 종료와 Level 종료 때는 네트워크 Player와 마지막 적용 tick을 정리한다.
- `NetworkClient::QueueInput`은 main thread에서 방향과 action flag를 받아 sequence를 부여하고, `BuildPacket_C2SInput`으로 만든 packet을 기존 outgoing queue에 넣는다. queue 삽입에 성공한 경우에만 sequence를 증가시킨다.
- `OverworldLevel`은 `Player::Tick`이 이번 frame의 이동 입력을 기록한 뒤 방향 변경과 공격 key-down edge만 `Game::SendNetworkInput`으로 전달한다. 방향키를 놓으면 `MoveDirection::None`을 한 번 보내 서버에 저장된 이동을 해제한다. 이번 단계에서는 기존 local Player 이동·공격도 그대로 실행한다.
- `OverworldLevel::Draw`는 기존 Renderer HUD 경로로 연결 상태를 표시한다. 표시 순서는 `[OFFLINE]` → `[Connecting...]` → `[ONLINE] Player <id>` → `[ONLINE] Player <id> Tick <tick> Num <count>`이다. network thread는 Renderer를 호출하지 않는다.

### 완료: Z1Shared wire layer 정리 (2026-08-29)

> `7dfccda`에서 중단됐던 공용 wire layer 리팩터링을 완료했다. 이후 Z1 클라이언트의 `C2S_Input` 전송까지 연결했다.

Rookiss의 buffer/packet 아이디어 중 raw memory 직렬화나 범용 Session/Service는 가져오지 않고, Z1Shared에 다음의 작은 공용 wire layer만 도입했다.

| 파일 | 현재 상태 | 최종 책임 |
| --- | --- | --- |
| `Z1Shared/Protocol.h` | 완료 | wire 계약 DTO와 enum만 선언. header를 raw struct로 전송하지 않음 |
| `Z1Shared/Serialization.h` | 완료 | network byte order를 지키는 `PacketReader`/`PacketWriter` |
| `Z1Shared/PacketCodec.h` | 완료 | 내부 `BuildPacket`과 Enter, Input, Player-only Snapshot의 builder/parser·payload 검증 |
| `Z1Shared/PacketFramer.h` | 완료 | TCP 누적 byte에서 완성 `Packet`을 하나씩 분리 |

#### 완료된 공유 코드 보강

1. `PacketWriter::TakeBytes()`에서 `const`와 `noexcept`를 제거했다. `const` 객체의 `_bytes`에 `std::move`를 적용하면 실제 move가 아니라 copy가 되고, 그 copy가 실패하면 `noexcept` 때문에 terminate할 수 있었기 때문이다.

   ```cpp
   std::vector<Byte> TakeBytes()
   {
       return std::move(_bytes);
   }
   ```

   `Bytes()`는 복사하지 않는 `std::span<const Byte>` view로 유지한다. Writer의 별도 offset 멤버는 필요 없다.

2. signed 32-bit 메서드는 기존 이름인 `Read32`/`Write32`를 유지하고 codec 양쪽에서 일관되게 사용한다.

3. `Serialization.h`는 WinSock에 의존하지 않는다. `EndianSwap`으로 정수를 big-endian 값으로 변환한 뒤 기존 `resize` + `memcpy` 구조로 byte vector에 기록하고, 읽을 때는 반대 순서로 복원한다.

4. packet header와 payload를 결합하는 `BuildPacket`은 typed codec에서만 사용하므로 `Serialization.h`에서 `PacketCodec.h` 내부 구현으로 옮겼다. Server/Client handler의 고정 payload 크기 중복 검사는 제거하고, 각 parser의 필드 읽기와 `IsAtEnd()` 검증으로 일원화했다. parser는 지역 변수에 모두 읽고 검증에 성공한 경우에만 출력 인자를 갱신한다.

5. `PacketFramer.h`는 `<Z1Shared/Serialization.h>`를 직접 include한다. framing 결과 타입 이름은 현재 용도에 맞게 `Packet`으로 확정했다.

   ```cpp
   struct Packet
   {
       PacketHeader header;
       std::vector<Byte> payload;
   };
   ```

6. Framer의 `TryPop`은 `bool`이 아니라 중첩 `PopResult`로 세 결과를 구분한다. 분할 수신의 `NeedMoreData`는 정상이고, size 오류의 `Invalid`는 연결 종료 대상이다.

   ```cpp
   enum class PopResult
   {
       Ready,
       NeedMoreData,
       Invalid
   };
   ```

#### PacketFramer 구현 계약

`PacketFramer`는 packet type이나 payload의 게임 의미를 알지 못한다. 다음만 책임진다.

1. `Append(bytes)`는 이전 미소비 byte 뒤에 새 recv byte를 붙인다. `BufferedSize() + bytes.size()`가 `MaxBufferedRecvBytes`를 넘으면 `false`를 반환한다.
2. 소비한 앞부분이 있으면 append 전에 한 번 compact한다. `_readOffset == _buffer.size()`이면 `clear`, 그 외에는 `[begin, begin + _readOffset)`만 erase하고 offset을 0으로 되돌린다.
3. `TryPop`은 남은 byte가 `PacketHeaderSize`보다 작으면 `NeedMoreData`를 반환한다.
4. `PacketReader`로 `PacketHeader.size`, `PacketHeader.type`을 읽고, `size < PacketHeaderSize` 또는 `size > MaxPacketSize`면 `Invalid`를 반환한다.
5. 완성 packet byte가 아직 `header.size`보다 적으면 `NeedMoreData`를 반환한다.
6. 충분하면 header 뒤 `header.size - PacketHeaderSize` byte를 `Packet::payload`로 복사하고 `_readOffset += header.size` 후 `Ready`를 반환한다.

확정한 public API는 다음이다.

```cpp
class PacketFramer
{
public:
    enum class PopResult
    {
        Ready,
        NeedMoreData,
        Invalid
    };

    bool Append(std::span<const Byte> bytes);
    PopResult TryPop(Packet& outPacket);
    std::size_t BufferedSize() const noexcept;

private:
    void CompactConsumedBytes();

    std::vector<Byte> _buffer;
    std::size_t _readOffset = 0;
};
```

`MaxBufferedRecvBytes`의 첫 값은 `MaxPacketSize * 2`다. caller가 매 `Append` 뒤 `TryPop`을 `NeedMoreData`까지 반복한다는 전제에서, 한 packet의 잔여 byte와 다음 recv를 동시에 보관할 수 있다. 이후 packet queue를 비동기로 별도 소비하게 되면 이 상한과 별도 ready-packet 상한을 다시 검토한다.

#### 완료된 Server 수신 경로 전환

`Z1Server/Session`은 기존 `_recvdData`를 제거하고 `PacketFramer _framer`를 소유한다. 수신 queue는 공용 `Packet`을 보관한다. `HandleRecv`의 순서는 다음과 같다.

```text
WSARecv completion byte
  → _framer.Append(recvBuffer span)
  → TryPop 반복
      Ready         : _recvdPackets에 move
      NeedMoreData  : HandleRecv 성공 반환
      Invalid       : HandleRecv 실패 반환
```

Server I/O loop의 `TryPopRecvdPacket` 구조는 유지하고, `HandleClientPacket`은 `packet.header.type`을 switch한다. `Server::HandleEnter`는 Player를 생성하기 전에 `ParsePayload_C2SEnter(payload, version)`을 호출해 protocol version까지 검증한다. `HandleInput`, S2C Enter, WorldSnapshot도 공용 codec을 사용한다.

#### 완료된 Client 수신 경로 전환

`NetworkClient`도 `_recvdData`를 `PacketFramer _framer`로 교체했다. `TryRecvPacketFromServer`는 recv 성공 뒤 `_framer.Append({ buffer.data(), bytesRead })`를 호출하고, `TryPop`을 `NeedMoreData`까지 반복한다. `Ready`마다 다음을 호출한다.

```cpp
HandleServerPacket(
    static_cast<PacketType>(packet.header.type),
    packet.payload
);
```

기존 `ProcessRecvdData()`의 수동 header parse, consumed 계산, `erase`는 제거했다. `HandleEnter`와 `HandleWorldSnapshot`은 각각 `ParsePayload_S2CEnter`, `ParsePayload_S2CWorldSnapshot`을 사용하고, Start의 `C2S_Enter`는 `BuildPacket_C2SEnter`로 생성한다.

#### 완료 내용과 검증

1. shared headers와 Server/NetworkClient의 공용 Framer 전환을 완료했다.
2. `Z1Server`와 `Z1`의 `Debug|x64` 솔루션 타깃 빌드가 성공했다.
3. `tools/test-z1-enter.ps1`을 확장해 정상 Enter/Snapshot, header 분할, payload 분할, Enter+Input 연속 전송, Input 이동·정지를 확인했다.
4. size `0`, `3`, `4097` packet과 protocol version `2`는 서버가 연결을 종료하며 거부했다.
5. 연속 Enter+Input에서 서버 Player의 y가 `395 → 387`, 일반 Input에서 `395 → 389`로 이동한 뒤 정지했다.
6. PowerShell 검증 뒤 실제 Z1 `NetworkClient`의 Enter/Snapshot/HUD와 `C2S_Input` 송신 경로도 통합 확인했다.

### 실제 확인 결과

- Enter packet을 한 번에 보내거나 header/body를 나누어 보내도 서버가 `Client Connected! → C2S_Enter received → Client Disconnected` 순으로 처리했다.
- PowerShell dummy client가 `S2C_Enter`의 10바이트 응답과 protocol version/playerId를 검증했다.
- `Up(sequence=1) → 300ms 유지 → None(sequence=2)` 입력에서 서버가 입력을 저장하고 Player y가 `395 → 388`로 이동한 것을 확인했다. 이 차이는 50ms tick 간격 동안 7회 실행된 결과다.
- Snapshot 수신에서 새 Player의 기본 상태 `position=(1190,395)`, `facing=Up(1)`, `hp=20`, `flags=0`, `Enemies=0`, `Projectiles=0`을 확인했다.
- PowerShell dummy client 두 개를 겹쳐 실행해 양쪽 snapshot에 `playerId=1,2`, `players=2`가 기록되고, 두 번째 연결 종료 뒤 남은 클라이언트 snapshot이 `players=1`로 돌아오는 것을 확인했다.
- 실제 Z1과 Z1Server를 함께 실행해 서버의 `Client Connected!`, `C2S_Enter received` 로그와 Z1 Overworld HUD의 `[ONLINE] Player 1 Tick 640 Num 1` 표시를 확인했다. 즉 실제 EXE에서도 connect → Enter → snapshot 수신 → main-thread HUD 반영 경로가 동작한다.
- 실제 Z1에서 방향키를 누르면 변경된 방향 packet 한 개가 전송되고, 키를 놓으면 `MoveDirection::None`이 전송되어 서버 이동이 멈추는 것을 확인했다. 방향 전환마다 sequence가 증가하고 서버 Player 좌표가 해당 방향으로 변했다.
- 검을 얻은 뒤 `A`를 누르면 해당 key-down 순간의 packet 하나에 `actionFlags=1`이 기록됐다. 이동 중 공격 입력도 같은 `InputCommand`에 함께 담기며, 현재 서버는 flag를 파싱·저장할 뿐 공격 simulation에는 아직 적용하지 않는다.
- 실제 Z1 두 개를 접속해 각 HUD에 서로 다른 local playerId와 `Num 2`가 표시되고, 각 클라이언트가 상대 playerId를 보라색 `NetworkPlayer`로 생성해 서버 snapshot 위치에 표시하는 것을 확인했다.
- 같은 PC에서 두 Z1을 실행하면 포커스와 관계없이 두 process의 `GetAsyncKeyState`가 같은 물리 키를 감지해 두 playerId가 함께 입력 packet을 보낸다. 이는 snapshot 복제 문제가 아니라 process별 입력 소유권 문제이며, 개선 방향은 [`CONSOLE_INPUT_DESIGN.md`](CONSOLE_INPUT_DESIGN.md)에 기록했다.

### 7일 플랜 기준 현재 위치

일주일 범위를 확정한 `25c3733` 이후 서버 payload vertical slice를 만든 뒤, 8월 28일 다음 네 커밋으로 실제 Z1 transport와 HUD 확인까지 연결했다.

```text
72bff3c  recv 누적 buffer와 TCP framing
77dd66c  S2C_Enter와 비동기 send queue
5b1a7bb  20Hz fixed tick
d53f8a0  C2S_Input과 ServerPlayerState
925304a  서버 권위형 Player 이동
d42e0a9  S2C_WorldSnapshot과 accepted socket queue
33de0e1  Socket Send/Recv와 TCP_NODELAY
8528b22  Z1 select 기반 NetworkClient
948f40f  Game의 NetworkClient 소유와 main-thread queue 소비
7bc882f  실제 Z1 접속 smoke test와 HUD
```

현재 진척은 기능 축에 따라 다음처럼 판단한다. 수치는 일정 관리를 위한 대략적인 값이며 완료 판정은 각 묶음의 실제 완료 기준을 우선한다.

| 작업 묶음 | 현재 상태 | 대략적 진척 |
| --- | --- | ---: |
| Socket payload 경로 | framing, Enter 왕복, partial send, send queue, 다중 dummy client 확인 완료. 안전한 종료 drain과 Session 제거 미완료 | 80~85% |
| 서버 OverworldSimulation | Player 상태·입력·기본 이동·Snapshot 완료. blocking map, Room, Enemy, Projectile, 전투 미완료 | 20~25% |
| Z1 NetworkClient와 Actor 표현 | select transport, Enter/Snapshot parsing, 입력 송신, 원격 `NetworkPlayer` 생성·갱신·제거 완료. local `MyPlayer` 분리 미완료 | 70~75% |
| 통합·검증 | PowerShell 다중 client와 실제 Z1 두 개의 접속·HUD·서버 좌표·원격 Actor 표현을 확인. 독립 키보드 입력은 콘솔 입력 개선 전까지 제한됨 | 35~40% |

전체 MVP의 관찰 가능한 기능 기준으로는 약 45% 지점이다. 서버 기준으로는 단계 4의 이동/Snapshot 기반을 만들었고, end-to-end 기준으로 실제 Z1 입력이 서버 시뮬레이션을 거쳐 다른 클라이언트의 `NetworkPlayer` 표현까지 도달한다. 다음 작업은 local `MyPlayer` 분리이며, 그 뒤 Overworld collision/Room, Enemy/Projectile과 전투가 남는다.

### 남아 있는 제약과 다음 재개 지점

- Z1은 기존 local `Player`의 방향 변경과 공격 key-down edge를 `C2S_Input`으로 보내지만, Overworld의 로컬 이동·공격·적 AI도 아직 그대로 실행한다. 원격 playerId는 서버 snapshot 기반 `NetworkPlayer`로 표현하지만 local playerId는 아직 `MyPlayer`로 전환하지 않았다.
- `NetworkPlayer`와 playerId→Actor map은 구현됐다. `OverworldLevel`이 main thread에서 원격 Player를 생성·갱신·제거하고 연결 종료 때 정리하지만, local playerId를 나타낼 `MyPlayer`는 아직 없다.
- CraftEngine의 `GetAsyncKeyState` polling 때문에 같은 PC에서 실행한 Z1 두 개가 같은 물리 키를 동시에 감지할 수 있다. `MyPlayer` 분리는 한 process 안의 입력 책임만 해결하므로, console focus별 입력 분리 방안과 `KEY_EVENT_RECORD` 후보는 별도 문서 [`CONSOLE_INPUT_DESIGN.md`](CONSOLE_INPUT_DESIGN.md)에서 다룬다. 입력 시스템을 개선하기 전 다중 client 조작 검증은 실제 Z1 하나와 dummy client를 함께 사용한다.
- MVP에서는 자동 재접속과 기존 Session 복구를 지원하지 않는다. 연결이 끊기면 Z1은 offline으로 전환하고 네트워크 표현을 정리하며, 다시 플레이하려면 클라이언트를 재시작한다. 새 연결은 새 Session과 새 playerId를 받으며, 계정·token으로 이전 ID나 상태를 복구하는 기능은 현재 계획에 포함하지 않는다.
- `NetworkClient`의 incoming queue가 가득 차면 현재는 연결을 끊는다. snapshot을 최신 하나로 합치는 정책은 실제 복제 표현이 동작한 뒤 필요할 때만 추가한다.
- Snapshot의 Enemy/Projectile 배열은 예약된 빈 배열이며, 공격 flag는 입력 검증만 한다. 공격·피격·HP 변화·사망·Enemy/Projectile simulation은 아직 없다.
- 서버 이동은 전체 Map 사각형 경계만 검사한다. `OverworldCollisionMap`의 blocking tile, Room 판정과 Room lifecycle은 아직 없다.
- `Session`은 닫힌 뒤 `_sessions` vector에서 아직 제거하지 않고 `playerId`도 유지한다. 따라서 이후 snapshot broadcast가 닫힌 entered Session에 다시 `Send`를 시도하고 `CloseSession`을 반복할 수 있으며, 장시간 실행하면 registry가 계속 커진다.
- closing 상태, outstanding I/O count, cancellation, pending completion drain, completion-port handle close를 갖춘 안전한 서버 종료 수명은 아직 구현하지 않았다. 현재 종료 경로를 최종 완료로 간주하지 않는다.
- `CompletionPort.h/.cpp`는 빈 stub이고 사용하지 않는다. 현재 `Server`가 raw completion-port `HANDLE`을 직접 소유한다.
- `S2C_Disconnect`는 packet type만 선언했고 아직 보내지 않는다.
- 계약에 적은 `MaxPlayers`, `MaxEnemies`, `MaxProjectiles`와 snapshot count 상한은 아직 구현하지 않았다. 현재 Player count는 `players.size()`를 `uint16_t`로 변환한다.
- 네트워크 작업 전 Z1, SokobanGame, ShootingGame의 Debug|x64 빌드·실행 기준선은 확인했다. 8월 28일 변경 뒤에는 Z1Server+Z1 실제 접속 smoke test를 확인했으나, 세 게임 전체 회귀 빌드·실행 기록은 아직 없다.
- 다음 재개 단위는 **local `MyPlayer` 분리**다. 네트워크 모드에서 기존 local `Player`의 판정 책임을 끊고, local playerId도 서버 snapshot을 위치 원본으로 사용하는 구조로 전환한다.

## 다음 구현 단위

Z1Shared wire layer, 공용 Framer 전환, 실제 Z1의 `C2S_Input` 전송과 원격 `NetworkPlayer` snapshot 반영은 완료됐다. 다음 재개 작업은 **local `MyPlayer` 분리**다.

1. `MyPlayer : NetworkPlayer`를 추가해 local playerId의 표시 객체와 입력 전송 책임을 나타낸다. 이동·충돌·공격 결과를 로컬에서 확정하지 않고 위치·방향·HP·flags는 서버 snapshot으로 적용한다.
2. 현재 `OverworldLevel::SendNetworkInput`의 입력 수집을 `MyPlayer`로 옮길지 검토하되, 동일 입력을 두 곳에서 읽지 않는다. 우선 구조를 단순하게 유지할 필요가 있으면 Level이 입력을 수집하고 `MyPlayer`는 local 표시 역할만 맡아도 된다.
3. 네트워크 모드에서는 기존 local `Player` 대신 local playerId에 해당하는 `MyPlayer`를 만들고, 나머지 ID에는 기존 `NetworkPlayer`를 사용한다. offline에서는 기존 `Player` 기반 싱글플레이 흐름을 유지한다.
4. 연결 종료 시 `MyPlayer`와 원격 `NetworkPlayer`를 모두 정리하고 offline 표현으로 돌아간다. 새 연결은 새 playerId로 새 객체를 만든다.
5. 실제 Z1 하나와 dummy client로 local/remote 생성·이동·정지·제거를 먼저 검증한다. 실제 Z1 두 개의 독립 키보드 조작은 [`CONSOLE_INPUT_DESIGN.md`](CONSOLE_INPUT_DESIGN.md)의 입력 개선 뒤 검증한다.

이 다음 구현 단위에서도 network thread는 기존 싱글플레이 `Player`, Enemy, Overworld Level을 직접 재사용하거나 변경하지 않는다. 서버의 blocking map/Room/전투와 안전한 Session 종료 수명은 이후 별도 단계로 남는다.
