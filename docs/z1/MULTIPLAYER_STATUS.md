# Z1 멀티플레이 개발 현황

마지막 갱신: 2026-09-01

## 문서 역할

이 문서는 현재 코드에서 구현·검증된 범위, 알려진 제약과 바로 다음 작업만 기록한다. 목표 계약은 [멀티플레이 설계](MULTIPLAYER_DESIGN.md)를 기준으로 한다.

## 현재 위치

TCP payload와 Player 이동 vertical slice에 A* 기반 Moblin 이동이 연결된 상태다. Z1의 실제 입력이 `C2S_Input`으로 서버에 도착하고, 서버가 20Hz Tick에서 BlockingMap 충돌을 통과한 Player 좌표와 수신 Player의 관심 Room Enemy를 `S2C_WorldSnapshot`으로 전송한다. local playerId는 `MyPlayer`, 원격 playerId는 `NetworkPlayer`로 표시한다. IOCP completion port HANDLE은 `CompletionPort`가 RAII로 소유한다.

네트워크 모드의 Player와 Enemy 위치·방향·HP는 서버 Snapshot이 원본이며, 클라이언트는 입력 의도만 보낸다. 서버 권위형 BlockingMap 충돌과 전역 Enemy 초기 스폰, Moblin의 Room 내부 A* 추적 이동은 적용됐고, 클라이언트는 local Snapshot 좌표로 배경과 View의 Room 표현을 바꾼다. Enemy 접촉·공격·피해와 Projectile은 아직 없다.

## 구현 완료

### 공용 socket과 wire 계층

- `Sockets` DLL의 `Net::Runtime`, `Net::Endpoint`, 이동 전용 `Net::Socket`
- Z1과 Z1Server의 Sockets 링크 및 DLL staging
- `Z1Shared`의 protocol enum, network byte order 직렬화, packet codec과 `PacketFramer`
- 4~4096 byte packet 크기 검증과 누적 수신 buffer 상한
- protocol version 2의 Enter, Input, WorldSnapshot codec
- Player 18-byte·Enemy 19-byte 배열과 count/남은 payload 길이 검증
- Enemy kind, facing, flags와 non-zero Enemy ID 검증

### Z1Server

- blocking accept thread와 accepted socket queue
- `CompletionPort`의 생성·association·dequeue·shutdown post와 HANDLE RAII
- CompletionPort에 연결되는 Session과 비동기 recv/send
- TCP 분할·연속 packet framing과 Session별 send queue
- `C2S_Enter`/`S2C_Enter`, playerId 할당과 protocol version 검증
- `C2S_Input` sequence·방향·flag parsing
- `BlockingMap.txt`의 256×88 형식과 `.`/`X` 통행 값 검증
- Player Box 전체 기준의 BlockingMap·전체 맵 경계 충돌
- 서버 시작 시 seed와 Room 좌표로 Enemy를 한 번 생성하고 전역 registry에 유지
- `Z1Server/Actor/Enemy`가 기존 `ServerEnemyState`를 대체하고 id·kind·`homeRoom`·spawn/current 좌표·방향·HP·사망 상태를 소유
- `Enemy::BuildSnapshot()`이 현재 서버 상태에서 `SnapshotEnemyState`를 작성
- 20Hz Tick에서 Player가 있는 active Room을 계산하고, 해당 Room의 살아 있는 Enemy만 갱신
- `RoomPathfinder`가 Enemy Box 기준의 16×11 `RoomNavigationGrid`에서 Manhattan distance A* 최단 경로 계산
- Moblin은 같은 `homeRoom`의 가장 가까운 살아 있는 Player를 대상으로, 4 Tick마다 경로의 다음 tile을 향해 한 server cell 이동
- Enemy 후보 위치는 `homeRoom` 좌표와 `CanPlaceEnemy()`를 통과한 뒤 `Enemy::MoveTo()`로 확정되며 다음 Snapshot에 반영
- entered Session마다 해당 Player의 관심 Room에 속한 Player·Enemy Snapshot을 작성

### Z1 클라이언트

- `Game`이 소유하는 select 기반 `NetworkClient`
- network thread와 main thread 사이의 bounded incoming/outgoing queue
- Enter/Snapshot parsing과 방향·공격 입력 전송
- HUD의 online 상태, local playerId, server tick과 player count 표시
- main thread에서 local `MyPlayer`와 원격 `NetworkPlayer` 생성·갱신·제거
- main thread에서 Snapshot의 `NetworkEnemy` 생성·갱신·제거
- `MyPlayer`만 입력을 읽어 방향 변화와 공격 edge를 `C2S_Input`으로 전송
- 온라인 중 기존 `Player`의 로컬 이동·공격·Enemy 처리와 충돌 판정을 실행하지 않음
- local `MyPlayer`의 서버 Snapshot 좌표가 다른 Room에 들어가면 Room 배경과 View 갱신
- 연결 종료 시 Player·Enemy 네트워크 표현을 제거하고 기존 싱글플레이 `Player` 흐름으로 복귀

## 확인한 동작

- Enter packet의 정상·header 분할·payload 분할 수신
- Enter와 Input의 연속 전송 및 이동 뒤 `None` 입력으로 정지
- size `0`, `3`, `4097`과 잘못된 protocol version의 연결 거부
- 두 dummy client의 서로 다른 playerId와 Snapshot player count 변화
- 실제 Z1과 Z1Server의 connect → Enter → Snapshot → HUD 반영
- 실제 Z1 방향키의 sequence 증가와 서버 좌표 변화
- 실제 Z1 두 개에서 상대 playerId의 `NetworkPlayer` 생성과 Snapshot 위치 반영
- 실제 Z1 하나와 KeepAlive dummy client에서 local `MyPlayer`와 원격 `NetworkPlayer` 생성, local 입력 전송과 서버 좌표 반영 확인
- 실제 Z1에서 BlockingMap 벽에 Player Box가 닿으면 서버 좌표가 더 이상 갱신되지 않음
- Z1Server와 Z1 `Debug|x64` 빌드 성공
- Enemy가 있는 Overworld Room에서 서버 Snapshot 기반 `NetworkEnemy` Sprite 표시 확인
- Moblin이 active Room의 Player를 대상으로 BlockingMap 장애물을 우회하는 A* 이동과 Snapshot 위치 갱신 확인
- 서버 종료 뒤 싱글플레이 fallback 시 현재 Room의 local Enemy가 한 번만 생성됨

재현 명령은 [Z1 검증](TESTING.md)에 기록한다.

## 알려진 제약

- 서버는 Player의 Room을 영구 상태로 보관하지 않는다. 수신 Player의 서버 좌표에서 관심 Room을 계산하고, 그 Room의 Player·Enemy만 Snapshot에 담는다.
- 접근 가능한 Room 경로 whitelist는 서버에 아직 없다. BlockingMap과 전체 맵 경계로 도달 가능한 모든 Room을 이동할 수 있다.
- 클라이언트 네트워크 Room 전환은 Snapshot Player 위치의 기준점으로 판정한다. 싱글플레이의 leading-edge 전환 규칙과 통일하는 작업은 남아 있다.
- A* 이동은 현재 Moblin에만 적용된다. 목표와 경로는 이동 기회마다 다시 계산하며, target/path cache는 아직 두지 않는다. Octorok과 Tektite는 정적 상태다.
- Enemy의 `homeRoom` 후보 검사는 좌상단 좌표 기준이다. `CanPlaceEnemy()`는 전체 8×5 Box를 검사하지만, Box 전체의 Room 경계 검사는 강화 대상이다.
- Player·Enemy 충돌, 접촉 피해, 공격, 사망과 Projectile은 아직 없다.
- Enemy 스폰은 BlockingMap의 전체 8×5 Box 통과 가능 여부만 검사한다. 같은 Room 안의 Enemy 위치 중복 방지는 아직 없다.
- Snapshot의 Projectile count는 0이다.
- 공격 flag는 edge로 전송되지만 서버는 parsing·저장만 하고 공격 판정에는 사용하지 않는다. 전투 구현 시 한 Tick에서 한 번만 소비해야 한다.
- 닫힌 Session을 `_sessions` registry에서 제거하는 최종 수명 처리가 완료되지 않았다.
- closing 상태, outstanding I/O, cancellation과 completion drain을 포함한 안전한 서버 종료가 완료되지 않았다.
- `S2C_Disconnect`는 선언만 되어 있다.
- 자동 재접속과 Session 복구는 지원하지 않는다.
- 같은 PC의 Z1 프로세스들이 동일한 물리 키를 함께 감지한다. 자세한 내용은 [콘솔 입력 설계](CONSOLE_INPUT_DESIGN.md)를 따른다.

## 다음 구현 단위

다음 작업은 Moblin을 기준으로 Projectile부터 공격 상호작용까지를 서버 권위 vertical slice로 연결하는 것이다. 이미 구현된 A* 이동은 유지하고, Octorok·Tektite AI 확장보다 전투의 공통 상태와 동기화 경계를 먼저 만든다.

1. `SnapshotProjectileState`와 codec, 클라이언트 `NetworkProjectile`을 추가해 서버 Projectile을 Snapshot으로 표시한다.
2. 서버 `Projectile` 상태에 id·owner·`homeRoom`·위치·방향·피해량·수명·사망 상태를 두고, Tick에서 이동·BlockingMap·Room 경계·수명 만료를 판정한다.
3. Moblin이 A* 목표를 향하는 동안 공격 cooldown 규칙에 따라 Projectile을 생성한다. 공격 대상과 방향은 서버 상태에서 결정한다.
4. Projectile-Player 충돌에서 서버가 HP 감소와 사망 상태를 확정하고 Player Snapshot으로 전파한다.
5. 이후 Player Sword-Enemy 충돌과 Enemy 사망 제거를 같은 규칙으로 추가하고, 두 Z1 클라이언트가 같은 결과를 보는지 확인한다.

Enemy·IOCP 확장 후보의 도입 조건과 보류 근거는 [네트워크 라이브러리 확장 검토 메모](NETWORK_LIBRARY_FOLLOWUPS.md)에 기록한다. 이후 순서는 Enemy Projectile과 공격 효과, Session 제거와 안전한 종료다. 완료되지 않은 항목을 구현된 현재 구조처럼 설계 문서에 옮겨 적지 않는다.
