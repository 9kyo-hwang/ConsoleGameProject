# Z1 멀티플레이 개발 현황

마지막 갱신: 2026-08-30

## 문서 역할

이 문서는 현재 코드에서 구현·검증된 범위, 알려진 제약과 바로 다음 작업만 기록한다. 목표 계약은 [멀티플레이 설계](MULTIPLAYER_DESIGN.md)를 기준으로 한다.

## 현재 위치

TCP payload와 Player 이동 vertical slice가 연결된 상태다. Z1의 실제 입력이 `C2S_Input`으로 서버에 도착하고, 서버가 20Hz Tick에서 Player 좌표를 변경한 뒤 `S2C_WorldSnapshot`으로 전송한다. 클라이언트는 원격 playerId를 `NetworkPlayer`로 표시한다.

로컬 Player는 아직 기존 싱글플레이 Actor와 판정을 사용한다. 따라서 현재 네트워크 경로는 서버 권위형 이동·전투가 완성된 상태가 아니다.

## 구현 완료

### 공용 socket과 wire 계층

- `Sockets` DLL의 `Net::Runtime`, `Net::Endpoint`, 이동 전용 `Net::Socket`
- Z1과 Z1Server의 Sockets 링크 및 DLL staging
- `Z1Shared`의 protocol enum, network byte order 직렬화, packet codec과 `PacketFramer`
- 4~4096 byte packet 크기 검증과 누적 수신 buffer 상한
- Enter, Input, WorldSnapshot codec과 Player count/payload 길이 검증

### Z1Server

- blocking accept thread와 accepted socket queue
- raw IOCP handle에 연결되는 Session과 비동기 recv/send
- TCP 분할·연속 packet framing과 Session별 send queue
- `C2S_Enter`/`S2C_Enter`, playerId 할당과 protocol version 검증
- `C2S_Input` sequence·방향·flag parsing
- 20Hz `OverworldSimulation`의 Player 이동과 전체 Player Snapshot broadcast

### Z1 클라이언트

- `Game`이 소유하는 select 기반 `NetworkClient`
- network thread와 main thread 사이의 bounded incoming/outgoing queue
- Enter/Snapshot parsing과 방향·공격 입력 전송
- HUD의 online 상태, local playerId, server tick과 player count 표시
- main thread에서 원격 `NetworkPlayer` 생성·갱신·제거

## 확인한 동작

- Enter packet의 정상·header 분할·payload 분할 수신
- Enter와 Input의 연속 전송 및 이동 뒤 `None` 입력으로 정지
- size `0`, `3`, `4097`과 잘못된 protocol version의 연결 거부
- 두 dummy client의 서로 다른 playerId와 Snapshot player count 변화
- 실제 Z1과 Z1Server의 connect → Enter → Snapshot → HUD 반영
- 실제 Z1 방향키의 sequence 증가와 서버 좌표 변화
- 실제 Z1 두 개에서 상대 playerId의 `NetworkPlayer` 생성과 Snapshot 위치 반영

재현 명령은 [Z1 검증](TESTING.md)에 기록한다.

## 알려진 제약

- local playerId를 위한 `MyPlayer`가 없고 기존 local `Player`가 이동·공격·충돌을 계속 판정한다.
- 서버 이동은 전체 맵 사각형 경계만 검사한다. BlockingMap, Room lifecycle과 active Room은 아직 없다.
- Snapshot의 Enemy와 Projectile count는 0이며 서버 전투 simulation이 없다.
- 공격 flag는 parsing·저장까지만 하고 공격 판정에 사용하지 않는다.
- 닫힌 Session을 `_sessions` registry에서 제거하는 최종 수명 처리가 완료되지 않았다.
- closing 상태, outstanding I/O, cancellation과 completion drain을 포함한 안전한 서버 종료가 완료되지 않았다.
- `CompletionPort` 타입은 빈 stub이며 현재 `Server`가 raw handle을 직접 소유한다.
- `S2C_Disconnect`는 선언만 되어 있다.
- 자동 재접속과 Session 복구는 지원하지 않는다.
- 같은 PC의 Z1 프로세스들이 동일한 물리 키를 함께 감지한다. 자세한 내용은 [콘솔 입력 설계](CONSOLE_INPUT_DESIGN.md)를 따른다.

## 다음 구현 단위

다음 작업은 local `MyPlayer` 분리다.

1. `MyPlayer : NetworkPlayer`를 추가해 local playerId의 표현 책임을 분리한다.
2. 네트워크 모드에서는 기존 local `Player` 대신 local Snapshot을 적용하는 `MyPlayer`를 사용한다.
3. 이동·공격 결과는 로컬에서 확정하지 않고 서버 Snapshot을 원본으로 삼는다.
4. 입력은 Level 또는 MyPlayer 중 한 곳에서만 읽어 `C2S_Input`으로 보낸다.
5. 연결 종료 시 MyPlayer와 원격 NetworkPlayer를 모두 정리하고 offline 흐름으로 돌아간다.
6. 실제 Z1 하나와 dummy client로 local/remote 생성·이동·정지·제거를 확인한다.

이후 순서는 BlockingMap과 Room, Enemy/Projectile과 전투, Session 제거와 안전한 종료다. 완료되지 않은 항목을 구현된 현재 구조처럼 설계 문서에 옮겨 적지 않는다.
