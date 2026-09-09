# Z1 네트워크 시뮬레이션 및 표시 보정 로드맵

마지막 갱신: 2026-09-09

상태: 제안. 아직 구현되지 않았으며, 다른 콘텐츠 기능보다 먼저 진행할 우선 작업 후보로 검토한다.

## 문서 목적

현재 Z1 멀티플레이는 localhost Loopback에서 서버 권위형 이동과 Snapshot 적용이 정상 동작하는 것을 중심으로 검증했다. 그러나 서버 Tick 지연, 네트워크 지연·jitter와 Snapshot 일괄 도착 상황에서 이동 속도와 클라이언트 표시를 안정화하는 처리는 아직 없다.

이 문서는 다음 두 문제를 하나의 선행 안정화 작업으로 정리한다.

1. 서버 Player 이동 속도를 Tick 호출 횟수가 아닌 초당 이동 거리로 정의한다.
2. 클라이언트가 서버 Snapshot을 수신 즉시 그대로 적용하는 구조에 원격 Actor 보간, 필요 시 제한적 외삽, local Player 예측·재조정을 단계적으로 추가한다.

현재 구현을 설명하는 문서는 [Z1 네트워크 아키텍처](NETWORK_ARCHITECTURE.md), 실행 검증 방법은 [Z1 테스트](TESTING.md)를 따른다. 이 문서의 항목은 완료 표시 전까지 현재 구현으로 간주하지 않는다.

## 우선순위 제안

다음 범위는 새로운 게임 규칙을 추가하기 전에 먼저 처리할 가치가 있다.

- 서버 Player의 이동 속도가 Tick 주기 변경에 따라 달라지는 문제 수정
- 지연·jitter·일괄 도착을 재현할 수 있는 최소 관찰 수단 추가
- 원격 Player·Enemy·Projectile의 Snapshot 보간

위 세 항목은 현재 네트워크 수직 슬라이스의 기본 시간 계약과 화면 표현을 안정화한다. 반면 local Player 예측과 원격 Actor 외삽은 복잡도와 오판 가능성이 더 크므로, 보간 적용 후 측정 결과에 따라 진행한다.

따라서 이 문서를 전체 기능보다 무조건 우선하는 확정 순서로 보지는 않는다. 우선 `1~3단계`를 네트워크 품질의 선행 기반으로 승격하고, `4~5단계`는 실제 RTT·jitter 테스트에서 필요성이 확인될 때 진행하는 것을 권장한다.

## 현재 구현과 확인된 한계

| 영역 | 현재 구현 | 한계 |
| --- | --- | --- |
| 서버 시간 | `IOLoop`가 50ms 간격의 20Hz Tick을 실행하고 최대 5회 catch-up | Tick 하나당 이동량을 사용하면 Tick 주기 변경 시 초당 이동속도가 달라짐 |
| Player 이동 | 서버 `OverworldSimulation::Tick()`에서 입력 방향으로 매 Tick 1셀 이동 | 현재 20Hz에서는 초당 20셀이지만 Tick rate에 종속됨 |
| Snapshot | 매 서버 Tick 뒤 `serverTick`과 관심 Room의 `ActorInfo` 배열을 전송 | catch-up 시 여러 Snapshot이 짧은 시간에 연속 생성될 수 있음 |
| 클라이언트 수신 | `NetworkClient`가 typed message queue에 넣고 `Game::PumpNetwork()`가 소비 | 여러 Snapshot을 소비하더라도 `Game`에는 마지막 Snapshot 하나만 남음 |
| Actor 적용 | `NetworkOverworldLevel::UpdateSnapshot()`이 `ApplySnapshot()`을 호출 | `NetworkPlayer`, `NetworkEnemy`, `NetworkProjectile`이 위치를 즉시 `SetPosition()`함 |
| local Player | `MyPlayer`가 입력 방향 변경과 공격 edge를 서버에 전송 | 로컬 이동 예측이 없어 화면 반응도 서버 왕복 시간에 의존함 |
| 검증 환경 | Loopback 중심 정상 동작 검증 | 실제 RTT, jitter, 재전송, 느린 수신자와 서버 Tick 지연 체감이 검증되지 않음 |

### 서버 지연과 네트워크 지연의 차이

서버 계산이 늦으면 `IOLoop`가 빠진 고정 Tick을 최대 5회 연속 실행한다. 이미 서버가 알고 있던 이동 입력은 각 catch-up Tick에 적용되고 Snapshot도 Tick마다 생성된다. 클라이언트가 마지막 Snapshot만 적용하면 중간 위치가 화면에 표시되지 않아 멈춘 뒤 여러 셀을 건너뛴 것처럼 보일 수 있다.

네트워크가 늦으면 서버 시뮬레이션은 정상적으로 진행해도 클라이언트가 과거 Snapshot을 표시한다. TCP와 Session 송신 큐는 순서를 보장하므로 최신 Snapshot이 오래된 Snapshot을 추월하지 않는다. 클라이언트 보정은 표시 품질을 개선할 수 있지만, 서버 송신 큐에 오래된 상태가 계속 누적되는 문제 자체를 해결하지는 않는다.

## 설계 원칙

### 서버 권위 유지

- 클라이언트 보간·외삽·예측 위치는 표시 또는 임시 예측 상태다.
- 충돌, Room, HP, 사망, 공격 판정과 최종 위치는 서버가 결정한다.
- 클라이언트가 계산한 위치를 서버에 전송하거나 서버 상태로 채택하지 않는다.

### 시뮬레이션 상태와 표시 상태 분리

- 서버 `ActorInfo`는 정수 월드 셀 좌표의 권위 상태를 유지한다.
- 이동 속도 계산에 필요한 소수 누적값은 서버의 구체 `Player` 내부에 둔다.
- 클라이언트의 수신 최신 상태와 화면에 표시 중인 상태를 분리한다.
- `Z1Shared`에는 wire contract와 `serverTick` 해석에 필요한 최소 계약만 둔다. 렌더링용 보간 객체를 넣지 않는다.

### 고정 Tick 유지

- 서버가 늦었더라도 한 번의 Tick에는 항상 동일한 고정 `deltaTime`을 사용한다.
- `now - previousTime` 같은 긴 실제 경과 시간을 한 Tick에 넘기지 않는다.
- catch-up은 동일한 고정 Tick을 여러 번 실행하는 방식으로만 수행한다.
- `MaxCatchupTicks`를 넘긴 시간은 현재 정책처럼 버리고 다음 Tick 기준 시각을 다시 잡는다.

### 보간은 유한한 지연을 대가로 사용

- 원격 Actor는 최신 서버 상태보다 일정 Tick 뒤를 표시해 짧은 jitter를 흡수한다.
- 보간 버퍼가 비면 미래의 권위 상태는 알 수 없으므로 잠시 정지하거나 제한적으로 외삽한다.
- 버퍼가 과도하게 커지면 모든 과거 상태를 끝까지 재생하지 않고 지연 상한 정책을 적용한다.

## 목표 데이터 흐름

```text
NetworkClient network thread
  TCP 수신·framing·역직렬화
             │
             ▼ typed message queue
Game::PumpNetwork main thread
  ├─ latest Snapshot: MyPlayer와 HUD의 최신 권위 상태
  ├─ pending Snapshots: 원격 Actor 표시용 순서 보존 상태
  └─ CombatEvent: 현재처럼 한 번씩 소비할 사건
             │
             ▼
NetworkOverworldLevel
  ├─ MyPlayer: 최신 상태 또는 예측·재조정 상태
  ├─ remote Snapshot playback buffer
  └─ NetworkPlayer·NetworkEnemy·NetworkProjectile 생성·갱신·제거
```

`NetworkClient`는 소켓과 패킷 처리만 담당하고 Actor를 변경하지 않는다. `Game`은 main thread로 넘어온 메시지를 종류별로 보관한다. 표시 시점, Room 전환과 ID 기반 Actor lifecycle은 현재와 같이 `NetworkOverworldLevel`이 담당한다.

## 0단계: 기준 동작 복원과 관찰 지점 확보

### 목적

보정 작업 전에 서버 Tick과 IOCP completion의 책임을 다시 고정한다.

### 구현 범위

- `Server::Tick()`은 20Hz 시간 검사와 `MaxCatchupTicks` 반복 안에서만 호출한다.
- IOCP recv/send completion 횟수가 시뮬레이션 Tick 횟수를 결정하지 않게 한다.
- 최소한 다음 값을 서버 로그 또는 임시 카운터로 관찰할 수 있게 한다.
  - Tick 실행 시간과 최대값
  - 한 IOLoop 반복에서 수행한 catch-up Tick 수
  - 건너뛴 Tick 시간 발생 횟수
  - Session별 송신 큐 high-water mark
- 클라이언트에는 다음 값을 임시 HUD나 로그로 표시할 수 있게 한다.
  - 마지막 수신 `serverTick`
  - 마지막 표시 `serverTick`
  - 표시 버퍼 크기
  - 버퍼 underrun과 오래된 Snapshot 폐기 횟수

### 완료 조건

- 입출력 completion이 많아져도 Tick은 20Hz 시간 기준으로만 실행된다.
- Loopback 기준 Tick이 정상적으로 증가하고 기존 이동·전투·Actor lifecycle이 유지된다.
- 이후 지연 테스트에서 서버 계산 지연과 클라이언트 표시 지연을 구분할 수 있다.

## 1단계: 서버 Player 이동 속도의 Tick 독립화

### 목표

현재 동작과 같은 초당 20셀을 기본값으로 유지하면서 Tick rate가 바뀌어도 같은 실제 시간 동안 거의 같은 거리를 이동한다.

### 권장 계산

```text
moveRemainder += moveCellsPerSecond × fixedDeltaSeconds
moveSteps = floor(moveRemainder)
moveRemainder -= moveSteps
```

예를 들어 `moveCellsPerSecond = 20`이면 다음 결과를 갖는다.

| Tick rate | 고정 Tick 시간 | Tick당 누적 이동량 | 1초 이동량 |
| --- | --- | --- | --- |
| 10Hz | 0.1초 | 2셀 | 20셀 |
| 20Hz | 0.05초 | 1셀 | 20셀 |
| 40Hz | 0.025초 | 0.5셀 | 20셀 |

40Hz에서는 첫 Tick에 실제 이동이 없고 다음 Tick에 1셀을 이동한다. 서버 Actor 좌표는 계속 정수 셀로 유지한다.

### 상태와 책임

- `moveCellsPerSecond`와 `moveRemainder`는 `Z1Server::Player`의 서버 전용 상태로 둔다.
- `Server::IOLoop`가 고정 Tick 시간을 알고 `Server::Tick(fixedDeltaSeconds)`에 전달한다.
- `Server::Tick()`은 같은 값을 `OverworldSimulation::Tick()`에 전달한다.
- `OverworldSimulation`은 Player에게 이동 가능한 정수 step 수만 요청하고, 기존 `CanPlacePlayer()`와 `SetPosition()` 경로로 한 셀씩 처리한다.
- 클라이언트 싱글플레이의 `CellStepComponent`와 개념은 같지만, Z1Server가 CraftEngine에 의존하지 않도록 해당 컴포넌트를 직접 사용하지 않는다.

### 경계 조건

- 여러 step을 이동하더라도 목적지 한 번만 검사하지 않고 매 셀마다 충돌을 검사한다.
- 벽에 막힌 이동 시 사용하지 못한 정수 step을 다음 Tick으로 이월하지 않는다.
- 입력이 `None`이 되었을 때 남은 소수 remainder를 유지할지 초기화할지 명시한다. 초기 구현은 재입력 직후 예상하지 못한 빠른 한 걸음을 막기 위해 초기화를 권장한다.
- 사망, 리스폰 또는 강제 위치 변경에서도 remainder를 초기화한다.
- Room 경계와 맵 바깥 좌표 검사는 기존 서버 권위 경로를 유지한다.

### 완료 조건

- 10Hz, 20Hz, 40Hz 시험 설정에서 2초간 같은 방향 입력 시 이동 거리 차이가 정수 양자화 범위인 1셀 이내다.
- 20Hz에서는 기존과 같은 초당 20셀 체감이 유지된다.
- 5회 catch-up은 긴 가변 `deltaTime` 한 번이 아니라 고정 Tick 5회로 처리된다.
- 벽 앞에서 오래 입력한 뒤 방향을 바꿔도 저장된 이동량 때문에 여러 셀을 갑자기 이동하지 않는다.

### 이 단계에서 하지 않는 일

- Enemy 이동·AI cooldown의 초 단위 변환
- Projectile 수명과 속도의 전면 변환
- 140 Tick 리스폰 같은 모든 Tick 기반 규칙의 변환

Player 이동 검증 후 실제 Tick rate 변경이 필요해질 때 나머지 Tick 기반 규칙을 별도 목록으로 전환한다.

## 2단계: 지연·jitter 재현과 기준 측정

### 목적

Loopback 정상 동작만으로 보간 효과를 판단하지 않고, 동일한 지연 조건을 반복해서 재현한다.

### 최소 시험 조건

| 조건 | 목적 |
| --- | --- |
| Loopback, 추가 지연 없음 | 기존 동작 회귀 기준 |
| 고정 지연 100ms | 일정한 RTT에서 추가 입력·표시 지연 확인 |
| 0~150ms jitter | 보간 버퍼가 짧은 변동을 흡수하는지 확인 |
| 250ms 수신 정지 후 일괄 전달 | 최대 5회 catch-up 또는 packet burst 체감 확인 |
| 500ms 이상 정지 | 버퍼 고갈과 보정 상한 확인 |
| Room 전환 중 지연 | 이전 Room Actor 잔존 여부 확인 |
| 사망·리스폰·Projectile 소멸 중 지연 | lifecycle을 보간 상태와 일치시키는지 확인 |

클라이언트 내부에서 Snapshot 전달만 지연시키는 debug queue는 화면 보간을 반복 검증하는 데 사용할 수 있다. 다만 이는 TCP backpressure와 서버 송신 큐 정체까지 재현하지 않으므로, 이후 다른 PC 또는 별도의 네트워크 조건 시험으로 보완한다.

### 측정값

```text
latestReceivedTick
displayedTick
snapshotBufferSize
bufferUnderrunCount
droppedSnapshotCount
correctionDistance
```

### 완료 조건

- 같은 시험 조건을 반복 적용하고 전후 영상을 비교할 수 있다.
- “서버 Tick이 늦음”, “Snapshot 수신이 늦음”, “클라이언트 표시 버퍼가 비어 있음”을 로그로 구분할 수 있다.

## 3단계: 원격 네트워크 Actor Interpolation

### 대상

- 다른 클라이언트의 `NetworkPlayer`
- `NetworkEnemy`
- `NetworkProjectile`

`MyPlayer`는 이 단계의 보간 지연 대상에서 제외한다.

### 버퍼 위치

- `Game::PumpNetwork()`는 현재처럼 network thread queue를 main thread에서 소비한다.
- WorldSnapshot을 `_latestSnapshot` 하나에만 덮어쓰지 않고 순서 보존 pending 목록에도 넣는다.
- `NetworkOverworldLevel`이 pending Snapshot을 가져와 표시용 bounded deque를 소유한다.
- Actor별 독립 Snapshot 큐를 먼저 만들지 않는다. 같은 `serverTick`의 Player·Enemy·Projectile과 생성·삭제를 함께 적용하기 위해 전체 WorldSnapshot 단위로 보관한다.

### 표시 기준

- 초기 목표 지연은 2 Tick, 즉 20Hz 기준 약 100ms로 둔다.
- 첫 진입 시 목표 개수의 Snapshot이 모일 때까지 원격 Actor 표시 시작을 잠시 기다린다.
- 클라이언트의 표시 clock은 서버 Tick 간격에 맞춰 진행한다.
- 같은 순간에 여러 Snapshot이 도착해도 수신 시각이 아니라 `serverTick` 순서와 고정 Tick 간격으로 재생한다.

현재 `Craft::Vector2`와 콘솔 좌표는 정수다. 인접한 두 셀 사이의 실수 위치를 계산해도 콘솔에서 그대로 표현할 수 없으므로, 초기 구현의 핵심은 공간 좌표 Lerp보다 시간축에서 Snapshot step을 고르게 재생하는 것이다.

### 최신 상태와 표시 상태

- `MyPlayer`, 연결 상태와 HUD는 최신 수신 Snapshot을 사용한다.
- 원격 Actor의 위치와 lifecycle은 표시 중인 Snapshot을 사용한다.
- 원격 Actor가 표시 Snapshot에서 처음 나타나면 해당 위치에 즉시 생성한다.
- 표시 Snapshot에서 사라졌을 때 제거한다. 최신 Snapshot에서 사라졌다는 이유만으로 과거 표시 시점의 Actor를 먼저 제거하지 않는다.

### 버퍼 고갈

- 다음 Snapshot이 없으면 초기 구현은 마지막 표시 위치에서 정지한다.
- 방향만 보고 계속 이동시키는 외삽은 이 단계에 넣지 않는다.
- 새로운 Snapshot이 도착하면 버퍼 정책에 따라 재생을 재개한다.

### 버퍼 과다 누적

모든 과거 Snapshot을 끝까지 20Hz로 재생하면 클라이언트가 서버보다 영구적으로 늦어질 수 있다.

- 목표 지연: 2 Tick
- 정상 범위보다 조금 많음: 표시 clock을 1.1~1.25배로 잠시 진행해 서서히 따라잡는 방안을 검토
- 허용 상한 초과: 오래된 Snapshot을 버리고 목표 표시 Tick 근처로 이동
- 연결마다 작은 고정 상한을 두고 무한 누적을 허용하지 않음

처음에는 복잡한 가변 속도 제어보다 “고정 목표 지연 + 명시적인 상한 초과 폐기”로 시작한다. 폐기 보정이 자주 보이면 그때 점진적 따라잡기를 추가한다.

### 불연속 상태

다음 상태는 과거 위치와 이어서 표현하지 않고 즉시 맞춘다.

- 첫 Snapshot과 Actor 최초 생성
- local Player의 Room 전환
- Enemy 리스폰
- 비정상적으로 큰 위치 차이
- Snapshot Tick 역행 또는 버퍼 재초기화

Room 전환 시 이전 Room의 표시 버퍼와 원격 Actor를 정리하고 새 Room Snapshot을 기준으로 다시 시작한다.

### CombatEvent 경계

`CombatEvent`는 유실하면 안 되는 사건이므로 Snapshot처럼 최신 하나로 합치지 않는다. 현재 CombatEvent에는 `serverTick`이 없어 원격 월드를 100ms 늦게 표시하면 공격 효과 시점과 표시 위치가 어긋날 수 있다.

초기 구현은 기존 이벤트 즉시 소비를 유지하고 실제 어긋남을 측정한다. 문제가 확인되면 별도 단계에서 CombatEvent에 서버 Tick을 포함하고 표시 clock에 맞춰 소비한다. 원격 위치 보간을 이유로 이벤트를 임의로 버리지 않는다.

### 완료 조건

- 정상 Loopback 동작이 회귀하지 않는다.
- 0~150ms jitter에서 원격 Actor가 마지막 Snapshot 위치로 즉시 여러 셀 점프하는 횟수가 감소한다.
- 250ms 일괄 도착 시 Snapshot을 동일 프레임에 모두 적용하지 않는다.
- Room 전환, 사망, 리스폰과 Projectile 제거 후 ghost Actor가 남지 않는다.
- 버퍼 고갈과 상한 초과가 카운터로 관찰된다.

## 4단계: MyPlayer Client-side Prediction과 Reconciliation

### 이 단계가 Interpolation·Extrapolation과 다른 이유

원격 Actor는 과거에 서버가 확정한 두 상태 사이를 보간한다. 반면 `MyPlayer`를 같은 방식으로 100ms 늦춰 표시하면 사용자 입력 반응이 더 나빠진다.

`MyPlayer`는 다음 흐름을 사용해야 한다.

```text
local input
→ 클라이언트가 즉시 예측 이동
→ 입력을 서버에 전송
→ 서버가 권위 위치와 처리한 입력 범위를 Snapshot으로 응답
→ 클라이언트가 서버 위치를 기준으로 미확정 입력을 다시 적용
→ 작은 표시 오차는 짧게 수렴, 큰 오차는 즉시 보정
```

### 필요한 상태 분리

```text
serverConfirmedPosition  서버가 확정한 기준 위치
predictedPosition        미확정 입력까지 적용한 클라이언트 예상 위치
renderPosition           화면에 실제 표시하는 위치
```

서버 판정은 항상 `serverConfirmedPosition`에서 온 Snapshot이 기준이다. `predictedPosition`은 서버에 위치로 전송하지 않는다.

### Protocol 검토 지점

현재 `InputCommand.sequence`와 서버 `Player::_lastInputSequence`는 존재하지만 WorldSnapshot에는 마지막 처리 입력 sequence가 없다. 수신자별로 생성하는 WorldSnapshot의 상위 필드에 `lastProcessedInputSequence`를 추가하는 방안을 검토한다. 모든 `ActorInfo`에 local Player 전용 필드를 반복해서 넣지 않는다.

현재 입력은 매 simulation Tick 명령이 아니라 방향 변경과 공격 edge가 있을 때 전송된다. 따라서 sequence ACK 하나만 추가해도 전형적인 입력 replay가 자동으로 완성되지는 않는다. 구현 전에 다음 중 하나를 선택한다.

1. 상태 변경 입력을 유지하고 클라이언트 예측 Tick과 입력 적용 구간을 별도로 기록한다.
2. 예측 단계에서 고정 주기의 입력 command를 보내 각 sequence가 한 예측 step과 명확히 대응하도록 입력 계약을 확장한다.

MVP 트래픽보다 재조정의 명확성이 중요해지는 시점에 결정하며, 이 문서에서는 아직 wire 변경안을 확정하지 않는다.

### 오차 보정

- 작은 위치 오차는 몇 client frame에 걸쳐 `renderPosition`을 `predictedPosition`으로 수렴시킨다.
- 큰 오차, Room 전환, 사망과 서버가 거부한 이동은 즉시 snap한다.
- 위치 보정 중에도 HP·사망·Room 같은 서버 권위 상태를 늦추지 않는다.
- 예측 이동 역시 현재 정수 셀 충돌 규칙과 동일한 순서로 처리해야 한다.

### 완료 조건

- 고정 RTT에서 키를 누른 직후 local Player가 화면상 반응한다.
- 서버 Snapshot 수신 후 미확정 입력을 반영해 예측 위치를 재구성할 수 있다.
- 벽, Room 경계와 방향 전환에서 작은 오차가 반복 진동하지 않는다.
- 큰 불일치가 발생해도 일정 시간 동안 잘못된 Room이나 충돌 위치를 유지하지 않는다.
- 다른 클라이언트에는 서버가 확정한 위치만 보인다.

## 5단계: 원격 Actor의 제한적 Extrapolation

### 진행 조건

3단계 보간을 실제 지연 환경에서 검증한 뒤에도 buffer underrun으로 원격 Actor 정지가 자주 보일 때만 진행한다. 보간 지연을 조금 늘리는 것만으로 충분하면 외삽을 추가하지 않는다.

### 기본 정책

- 보간 가능한 다음 Snapshot이 없을 때만 외삽한다.
- 최대 1~2 Tick, 즉 50~100ms 정도로 제한한다.
- 마지막 두 Snapshot의 위치 변화와 방향을 근거로 한다.
- 제한 시간을 넘기면 마지막 예상 위치에서 정지한다.
- 새 Snapshot이 도착하면 예측 위치와 권위 위치 차이를 계산한다.
  - 작은 오차: 짧게 수렴
  - 큰 오차: 즉시 snap

### Actor별 위험

| Actor | 외삽 가능성 | 주요 위험 |
| --- | --- | --- |
| 원격 NetworkPlayer | 비교적 높음 | 키 해제·방향 전환·벽 충돌을 늦게 알 수 있음 |
| NetworkProjectile | 짧은 구간만 가능 | 서버에서 충돌·수명 만료로 이미 제거됐을 수 있음 |
| NetworkEnemy | 초기에는 보류 권장 | A* waypoint 변경, 충돌과 공격 상태를 예측하기 어려움 |

한 번에 모든 원격 Actor를 일반화하지 않는다. 필요하면 원격 Player부터 짧게 적용하고 Projectile, Enemy 순서로 실제 오차를 확인한다.

### 외삽하지 않는 상태

- Actor 생성·삭제·사망·리스폰
- Room 전환
- 직전 Snapshot에서 위치가 불연속적으로 변함
- 방향이나 이동 여부를 신뢰할 수 없음
- 외삽 제한 시간 초과

### 완료 조건

- 짧은 buffer underrun에서 원격 Player의 즉시 정지 현상이 감소한다.
- 외삽 때문에 벽을 오래 통과하거나 제거된 Projectile이 장시간 남지 않는다.
- 보정 거리와 snap 횟수를 측정할 수 있다.
- 외삽을 끄면 3단계 보간 상태로 즉시 돌아갈 수 있다.

## 별도 후속: 서버 Snapshot 신선도 정책

클라이언트 보간·외삽은 이미 네트워크를 통과한 상태의 표시를 개선한다. Session 송신 큐와 TCP 버퍼에 오래된 Snapshot이 누적되는 문제는 서버에서 별도로 다뤄야 한다.

측정 결과 느린 수신자에서 과거 WorldSnapshot이 지속적으로 쌓인다면 다음 정책을 검토한다.

- 전송 중인 front packet의 메모리는 변경하지 않는다.
- 아직 `WSASend`하지 않은 오래된 WorldSnapshot만 최신 Snapshot으로 합친다.
- `S2C_Enter`, `CombatEvent`와 같은 순서 보존 사건은 합치거나 버리지 않는다.
- 패킷 타입을 모르는 단순 byte queue 위에 임의 덮어쓰기를 추가하지 않는다.

이는 보간 구현의 선행 조건이 아니며 Session 송신 큐 high-water mark와 실제 지연을 측정한 뒤 별도 작업으로 진행한다.

## 단계별 변경 예상 위치

| 단계 | 주요 파일 | 역할 |
| --- | --- | --- |
| 0 | `Z1Server/Server.cpp`, `Z1Server/Session.*` | Tick 기준 복원과 관찰 지점 |
| 1 | `Z1Server/Server.*`, `Z1Server/OverworldSimulation.*`, `Z1Server/Actor/Player.*` | 고정 dt 전달과 이동 step 누적 |
| 2 | `Z1/Network/NetworkClient.*` 또는 debug 전용 경로, `Z1/Network/NetworkOverworldLevel.*` | 지연 재현과 표시 지표 |
| 3 | `Z1/Game/Game.*`, `Z1/Network/NetworkOverworldLevel.*` | Snapshot 순서 보존과 원격 표시 clock |
| 4 | `Z1Shared/Protocol.h`, `Z1Shared/PacketCodec.h`, `Z1Server/Actor/Player.*`, `Z1/Network/MyPlayer.*` | 입력 ACK, 예측 이력과 재조정 |
| 5 | `Z1/Network/NetworkOverworldLevel.*`와 필요한 원격 Actor | 제한적 외삽과 오차 보정 |

새 `.cpp`나 `.h`를 추가할 필요가 확인되면 Z1 프로젝트의 `.vcxproj`와 `.vcxproj.filters`도 함께 갱신한다. 초기 보간은 기존 Game·Level 책임 안에서 구현해 새 범용 네트워크 보간 계층을 먼저 만들지 않는다.

## 전체 완료 기준

- 서버 Player 이동 속도가 Tick rate에 비례해 변하지 않는다.
- 원격 Actor가 짧은 jitter와 Snapshot 일괄 도착에서 같은 프레임에 여러 셀을 건너뛰는 현상이 감소한다.
- local Player는 예측을 도입하기 전까지 최신 서버 상태를 사용하고, 예측 도입 후에는 입력 ACK 기준으로 재조정된다.
- 보간·외삽·예측이 서버 권위 충돌, HP, 사망, Room과 Actor lifecycle을 변경하지 않는다.
- 버퍼 크기, underrun, 폐기와 보정 거리를 관찰할 수 있다.
- Loopback뿐 아니라 인위적 지연·jitter와 다른 PC 환경에서 수동 검증한다.
- `Z1`, `Z1Server`의 `Debug|x64` 빌드와 관련 packet parser 검증을 통과한다.
- 구현된 단계만 `NETWORK_ARCHITECTURE.md`의 현재 구조에 반영하고, 검증 방법은 `TESTING.md`에 추가한다.

## 보류 항목

다음은 측정 없이 먼저 추가하지 않는다.

- 모든 Actor가 공유하는 범용 interpolation/extrapolation framework
- 무제한 외삽
- local Player에 원격 Actor와 같은 고정 표시 지연 적용
- client position을 서버가 권위 값으로 수락하는 처리
- 동적 Tick rate negotiation
- UDP 전환, Delta Snapshot, lock-free queue와 IOCP worker pool
- 보간만을 위해 CraftEngine Transform 좌표를 실수형으로 전면 변경
