# Z1 멀티플레이 설계

## 문서 역할

이 문서는 Z1 서버 권위형 멀티플레이의 확정 경계와 완료 조건을 정의한다. 구현 여부, 검증 결과와 다음 작업은 [멀티플레이 현황](MULTIPLAYER_STATUS.md)에만 기록한다. 전체 솔루션의 프로젝트 관계는 [솔루션 아키텍처](../ARCHITECTURE.md)를 따른다.

## MVP 범위

- 두 개 이상의 Z1 클라이언트가 하나의 localhost Z1Server에 접속한다.
- 서버가 Player 이동, 공격, 피격, HP와 사망을 결정한다.
- 서버가 Overworld의 Enemy와 Projectile 상태를 결정한다.
- 클라이언트는 Snapshot으로 Player·Enemy·Projectile 표현을 생성·갱신·제거한다.
- Room 전환은 서버가 확정한 전체 맵 월드 좌표를 기준으로 한다.
- 네트워크 비활성 또는 연결 실패 시 기존 싱글플레이를 유지한다.

이번 범위에는 Dungeon/Cave/보스·아이템 동기화, 로그인·DB·로비, 자동 재접속, 클라이언트 예측·보간, UDP와 범용 Engine NetworkComponent를 포함하지 않는다.

## 책임 경계

```text
Sockets DLL
  WinSock runtime, Endpoint, 이동 전용 Socket, 기본 socket 연산

Z1Shared headers
  PacketHeader, PacketType, 직렬화, packet codec, TCP framing

Z1Server EXE
  accept, IOCP Session, send queue, packet dispatch
  20Hz OverworldSimulation, 권위형 객체 상태, Snapshot broadcast

Z1 EXE
  NetworkClient의 select thread와 송수신 queue
  main thread의 Snapshot 소비와 표현 Actor 관리
```

`Sockets`는 IOCP, Session, Z1 packet 또는 게임 규칙을 알지 않는다. `Z1Server`는 CraftEngine, Renderer, Input, Sound와 Z1 Actor를 링크하지 않는다. `Z1Shared`에는 Sprite, Actor, Level이나 서버 Session 타입을 넣지 않는다.

## 서버와 클라이언트 상태

목표로 하는 서버의 최소 권위 상태는 다음과 같다.

```text
Player: playerId, position, facing, hp, dead, input sequence, action state
Enemy:  networkId, kind, position, facing, hp, action state
Projectile: networkId, kind, ownerId, position, direction, lifetime
```

클라이언트의 네트워크 표현은 로컬 판정을 실행하지 않는다.

```text
NetworkPlayer       서버 Snapshot의 Player 표현
└─ MyPlayer         local playerId 표현과 입력 전송 책임

NetworkEnemy        서버 Enemy 표현
NetworkProjectile   서버 Projectile 표현
NetworkSwordEffect  공격 상태의 시각·사운드 표현
```

기존 `Player`, `Enemy`, `Projectile`, `SwordAttack`은 싱글플레이 AI·충돌·피해 판정을 포함하므로 네트워크 표현 타입으로 재사용하지 않는다.

## Wire protocol

TCP 위에 고정 4바이트 header를 사용한다.

```text
[size:uint16][type:uint16][payload...]
```

- 모든 다중 byte 정수는 network byte order다.
- `size`는 header를 포함하며 허용 범위는 4~4096 byte다.
- protocol version은 현재 `1`이다.
- 수신 byte는 `PacketFramer`에 누적하고 완성된 packet만 handler에 전달한다.
- header/payload 분할 수신과 한 번에 여러 packet을 받은 경우를 모두 처리한다.
- 잘못된 size, version, enum, flag 또는 payload 길이는 연결 오류로 처리한다.

현재 protocol에 선언된 packet 종류는 다음과 같다. 선언 여부와 실제 payload 구현 범위는 다를 수 있으며, 구현 완료 범위는 [멀티플레이 현황](MULTIPLAYER_STATUS.md)을 따른다.

| 방향 | Packet | 역할 |
| --- | --- | --- |
| C→S | `C2S_Enter` | protocol version 제시와 입장 요청 |
| S→C | `S2C_Enter` | protocol version과 local playerId 할당 |
| C→S | `C2S_Input` | sequence, 이동 방향과 공격 edge |
| S→C | `S2C_WorldSnapshot` | server tick과 객체 상태 배열. 현재는 Player만 직렬화하며 Enemy·Projectile은 확장 목표 |
| S→C | `S2C_Disconnect` | 서버 주도 연결 종료 통지용 예약 packet |

목표 wire format에서 Player, Enemy, Projectile은 별도 packet 종류가 아니라 하나의 전체 상태 Snapshot에 포함한다. 현재 codec은 Player 배열 뒤에 Enemy와 Projectile의 count를 예약해 두었으며 두 값이 모두 0인 packet만 허용한다. 객체 payload를 추가할 때는 count와 남은 길이를 객체 접근 전에 검증하고 전체 packet 크기에서 계산한 상한을 넘지 않는다.

## 동시성과 소유권

### Z1 클라이언트

- `Game`이 `NetworkClient`를 값으로 소유한다.
- network thread는 socket, framing과 송수신 queue만 다룬다.
- main thread는 incoming message를 소비하고 Level/Actor를 변경한다.
- outgoing queue와 incoming queue에는 상한을 둔다.
- 연결 종료 시 thread를 깨우고 join한 뒤 socket과 표현 Actor를 정리한다.

### Z1Server

- accept thread는 accepted socket을 queue에 넣을 뿐 Session registry나 simulation을 변경하지 않는다.
- Server loop가 accepted socket을 Session으로 만들고 completion port에 연결한다.
- 비동기 operation은 completion이 처리될 때까지 `OVERLAPPED`와 buffer 수명을 유지한다.
- Session별 send queue는 packet 순서와 partial send를 보존한다.
- 닫힌 Session은 outstanding I/O가 정리된 뒤 registry와 simulation에서 한 번만 제거한다.
- 서버 종료 시 accept와 server thread를 join하고 pending completion을 정리한 뒤 completion port를 닫는다.

초기 구현은 IOCP completion과 20Hz simulation을 한 Server loop가 처리한다. 측정된 지연이나 처리량 문제가 생기기 전에는 worker pool, simulation thread 분리, lock-free queue나 shard를 도입하지 않는다.

## 권위형 Overworld

클라이언트는 방향과 공격 의도만 보내고 위치·HP·아이템 상태를 보내지 않는다. 서버는 sequence와 입력 범위를 검증하고 고정 Tick에서 다음을 결정한다.

1. Player 이동과 맵·blocked 충돌
2. 전체 맵 좌표에서의 Room 소속
3. active Room의 Enemy 생성과 Tick
4. 공격·Projectile·피격·HP·사망
5. 모든 active Room의 전체 Snapshot

첫 구현은 Player가 있는 Room만 active 상태로 유지한다. 마지막 Player가 나간 Room의 Enemy와 Projectile은 제거하고 다시 입장하면 결정적인 계획으로 생성한다. 객체 수가 작으므로 관심 영역 packet을 별도로 만들지 않고, 클라이언트가 자신의 Room에 필요한 표현만 선택한다.

서버에 맵 충돌을 구현할 때는 `Content/Z1/Maps/Overworld/BlockingMap.txt`의 통행 데이터만 읽도록 한다. TileId, Sprite와 Color는 읽지 않으며 CraftEngine Tilemap을 링크하지 않는다. 클라이언트와 서버의 중복 parser가 실제 유지보수 문제가 될 때만 렌더링 독립적인 MapData 공유를 검토한다. 현재 구현 여부는 [멀티플레이 현황](MULTIPLAYER_STATUS.md)을 기준으로 판단한다.

## 신뢰 경계

- packet 크기와 count를 buffer 접근 전에 검증한다.
- enum, flag, 좌표와 sequence 범위를 검증한다.
- 연결에 할당된 playerId 외의 객체를 조작하지 못하게 한다.
- packet 빈도와 queue 크기에 상한을 둔다.
- malformed packet을 반복하거나 상한을 넘긴 Session은 종료한다.
- localhost 학습 서버를 인증·암호화·서비스 거부 대응 없이 공용 인터넷에 노출하지 않는다.

## 완료 기준

MVP는 실제 Z1 클라이언트 두 개에서 이동·Room·전투·HP·사망과 Enemy/Projectile 결과가 서버 기준으로 같고, 한 클라이언트의 종료와 재접속이 다른 Session이나 서버 월드에 영향을 주지 않을 때 완료다. 같은 PC의 독립 키 입력 문제는 [콘솔 입력 설계](CONSOLE_INPUT_DESIGN.md)의 별도 완료 조건을 따른다.
