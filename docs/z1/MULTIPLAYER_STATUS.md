# Z1 멀티플레이 개발 현황

마지막 갱신: 2026-09-04

## 문서 역할

이 문서는 현재 코드에서 구현·검증된 범위, 알려진 제약과 바로 다음 작업만 기록한다. 목표 계약은 [멀티플레이 설계](MULTIPLAYER_DESIGN.md)를 기준으로 한다.

## 현재 위치

Title에서 `LocalPlay`와 `MultiPlay` 선택 메뉴가 분리되었으며, 기존 싱글플레이는 순수 `OverworldLevel`을 사용하고 멀티플레이는 전용 `NetworkOverworldLevel`로 분리되었다. 기존 인게임 offline fallback 코드는 완전히 제거되었으며, 연결 끊김 및 Player 사망 시 네트워크 상태를 정리하고 Title로 안내 메시지와 함께 복귀한다.

TCP payload와 Player 이동 vertical slice에 A* 기반 Moblin 이동과 적 Projectile 공격이 연결된 상태다. Z1의 실제 입력이 `C2S_Input`으로 서버에 도착하고, 서버가 20Hz Tick에서 BlockingMap 충돌을 통과한 Player 좌표와 수신 Player의 관심 Room에 속한 Enemy·Projectile을 `S2C_WorldSnapshot`으로 전송한다. 서버가 Moblin의 실제 A* 최종 경로를 `S2C_EnemyPathDebug`로 전송하고 클라이언트가 `F3` 토글로 표시한다. local playerId는 `MyPlayer`, 원격 playerId는 `NetworkPlayer`로 표시한다. `CraftEngine::Input`이 콘솔 입력 버퍼 기반으로 전환되어 동일 PC에서 여러 Z1 클라이언트를 실행해도 포커스된 콘솔 창의 입력만 독립적으로 처리된다. IOCP completion port HANDLE은 `CompletionPort`가 RAII로 소유한다.

네트워크 모드의 Player·Enemy·Projectile 위치와 HP·사망은 서버 Snapshot이 원본이며, 클라이언트는 입력 의도만 보낸다. 서버 권위형 BlockingMap 충돌과 전역 Enemy 초기 스폰, Moblin의 Room 내부 A* 추적 이동·Spear 발사, Projectile의 Player 피격·사망, Player 일반 검의 Enemy 피격·사망까지 적용됐다. 클라이언트는 local Snapshot 좌표로 배경과 View의 Room 표현을 바꾸고 Snapshot에서 빠진 사망 Enemy 표현을 제거한다. 서버가 승인한 일반 검 공격은 `S2C_CombatEvent`로 같은 Room의 클라이언트에 전파하며, 각 클라이언트가 표현 전용 `NetworkSwordEffect`와 공격 효과음을 재생한다. 최대 HP SwordBeam은 아직 없다.

## 구현 완료

### 공용 socket과 wire 계층

- `Sockets` DLL의 `Net::Runtime`, `Net::Endpoint`, 이동 전용 `Net::Socket`
- Z1과 Z1Server의 Sockets 링크 및 DLL staging
- `Z1Shared`의 protocol enum, network byte order 직렬화, packet codec과 `PacketFramer`
- 4~4096 byte packet 크기 검증과 누적 수신 buffer 상한
- protocol version 4의 Enter, Input, WorldSnapshot, CombatEvent, EnemyPathDebug codec
- Player 18-byte·Enemy 19-byte·Projectile 14-byte 배열과 count/남은 payload 길이 검증
- Enemy/Projectile kind, facing/direction, flags와 non-zero 객체 ID 검증
- 6-byte CombatEvent payload의 event type, actorId, cardinal direction과 정확한 payload 길이 검증

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
- Moblin이 실제 `RoomPathfinder::FindPath()` 결과를 Enemy ID별 최신 `EnemyPathDebug`로 기록하고, 관심 Room의 entered Session에 전송
- Enemy 후보 위치는 `homeRoom` 좌표와 `CanPlaceEnemy()`를 통과한 뒤 `Enemy::MoveTo()`로 확정되며 다음 Snapshot에 반영
- 서버 `Projectile`이 id·kind·owner Enemy id·`homeRoom`·위치·방향·피해량·남은 수명을 소유
- Moblin이 공격 cooldown에 따라 가장 가까운 살아 있는 Player 방향으로 Spear를 생성
- active Room의 Projectile을 한 server cell씩 이동하고 BlockingMap, Room 경계, 수명 만료 시 registry에서 제거
- Projectile의 다음 위치와 Player Box를 AABB로 검사해 충돌 시 Player HP를 감소시키고 Projectile을 제거
- HP가 0이 된 Player는 dead 상태로 전환하고 이동 입력을 정지하며 Enemy 추적 대상에서 제외
- `C2S_Input`의 `InputActionAttack`을 지속 이동 상태와 분리된 `attackRequested`로 보관하고 다음 Tick에서 한 번만 소비
- 이동 입력이 있으면 이동만 처리하고, 정지 상태의 공격 요청에만 방향 기준 Sword AABB 판정 적용
- 일반 검은 좌우 10×3·상하 6×5 Box로 같은 Room의 살아 있는 Enemy를 검사하고, 겹친 대상 중 가장 작은 network ID 하나에 피해 1 적용
- `Enemy::TakeDamage()`가 실제 피해량만큼 HP를 낮추고 HP 0에서 dead 상태 확정
- dead Enemy는 전역 registry에 유지하되 Tick과 일반 Snapshot에서 제외
- 서버 Tick에서 승인된 정지 일반 검 공격마다 Player id·facing과 관심 Room을 `PendingCombatEvent`로 한 번 기록
- Tick 뒤 쌓인 CombatEvent를 꺼내 공격 Player와 같은 관심 Room의 entered Session에 전송하며 공격한 Session도 수신 대상에 포함
- entered Session마다 해당 Player의 관심 Room에 속한 Player·Enemy·Projectile Snapshot을 작성
- `CloseSession()`이 Session을 `_closing`으로 한 번만 전환하고, `RemovePlayer()`와 socket close를 중복 호출하지 않음
- Recv/Send `OVERLAPPED` completion을 IOLoop에서 확인해 `_recvPending`/`_sendPending`을 해제하고, closing Session은 새 I/O와 Broadcast에서 제외
- IOLoop 반복문 시작의 `RemoveClosedSessions()`가 pending I/O가 남지 않은 Session만 `_sessions`에서 제거

### Z1 클라이언트

- `Game`이 소유하는 select 기반 `NetworkClient`
- network thread와 main thread 사이의 bounded incoming/outgoing queue
- `NetworkClient::Stop()`이 thread join 뒤 socket, 양방향 queue, pending 전송 packet, framer, partial-send offset과 input sequence를 초기화
- `NetworkClient::Start()`의 socket 연결·설정과 `C2S_Enter` 생성·queue 실패 경로에서 socket close
- Enter/Snapshot/CombatEvent parsing과 방향·공격 입력 전송
- HUD의 online 상태, local playerId, server tick과 player count 표시
- main thread에서 local `MyPlayer`와 원격 `NetworkPlayer` 생성·갱신·제거
- main thread에서 Snapshot의 `NetworkEnemy` 생성·갱신·제거
- main thread에서 Snapshot의 `NetworkProjectile` 생성·위치 갱신·제거
- `MyPlayer`만 입력을 읽어 방향 변화와 공격 edge를 `C2S_Input`으로 전송
- 온라인 중 기존 `Player`의 로컬 이동·공격·Enemy 처리와 충돌 판정을 실행하지 않음
- local `MyPlayer`의 서버 Snapshot 좌표가 다른 Room에 들어가면 Room 배경과 View 갱신
- Snapshot HP와 dead flag를 `MyPlayer`와 HUD에 반영하고 dead 상태에서는 입력 전송을 중지
- `Game`이 순서가 중요한 CombatEvent를 별도 queue에 모두 보관하고 `NetworkOverworldLevel`이 main thread에서 소비
- `Game`이 Enemy ID별 최신 `EnemyPathDebug`를 map에 보관하고, `NetworkOverworldLevel`이 현재 Room의 경로를 main thread에서 렌더링
- `F3` 토글로 Enemy 경로 디버그 표시를 켜고 끄며, Enemy 사망·Room 이동 시 빈 경로와 Room 필터로 이전 표시를 제거
- 일반 검 이벤트의 actorId로 `MyPlayer` 또는 `NetworkPlayer`를 찾고, facing별 로컬 오프셋에 표현 전용 `NetworkSwordEffect`를 부착
- `NetworkSwordEffect`는 충돌·피해 판정 없이 검 Sprite를 0.5초 표시한 뒤 제거되며 각 수신 클라이언트가 검 공격 효과음을 한 번 재생

### CraftEngine 입력 시스템 및 다중 클라이언트 분리

- `CraftEngine::Input`을 `GetAsyncKeyState` 전역 polling에서 `STD_INPUT_HANDLE` 기반 `KEY_EVENT_RECORD` 이벤트 소비로 전환
- `GetNumberOfConsoleInputEvents`로 대기 이벤트 개수를 확인한 뒤 `ReadConsoleInputW`로 버퍼를 일괄 drain하여 논블로킹 처리
- `KeyState`를 `held`, `pressed`, `released`로 분리해 키 auto-repeat 시 `GetKeyDown` 중복 방지 및 1프레임 내 빠른 press/release 엣지 보존
- `FOCUS_EVENT`(`!bSetFocus`) 수신 시 눌려 있던 모든 키를 `released`로 전환해 창 전환 시 이동 멈춤(`MoveDirection::None`) 보장
- `Engine::SavePreviousInputStates()` 및 `Input::SaveKeyStates()`를 제거하고 매 프레임 `ReadConsoleInputEvents()`로 상태를 갱신
- `CraftEngine::Input`의 기존 공개 API(`GetKey`, `GetKeyDown`, `GetKeyUp`) 유지

### 플레이 모드 분리와 NetworkOverworldLevel 및 종료·사망 복귀

- TitleLevel에 `LocalPlay`, `MultiPlay` 위/아래 선택 메뉴 구현 (기본 선택은 `LocalPlay`)
- Local Play 선택 시 `NetworkClient::Start()`를 호출하지 않고 순수 싱글플레이 `OverworldLevel` 실행
- Multiplayer 선택 시 `Connecting` 상태를 거쳐 서버 연결 및 `S2C_Enter` 확인 후 전용 `NetworkOverworldLevel` 진입
- `OverworldLevel`에서 네트워크 Actor, 패킷 소비, offline fallback 관련 잔재(`_wasOnline`, `EnsureOfflinePlayers`, `fallbackPosition` 등)를 전면 제거하여 싱글플레이 전용으로 정제
- 멀티플레이 전담 `NetworkOverworldLevel`을 신설하여 Snapshot 갱신, 원격 Actor 관리, CombatEvent 적용, Enemy 경로 디버그 렌더링을 격리
- `Game::OnDisconnect`를 통해 `NetworkClient` transport 상태와 `Game`의 수신 상태 정리를 일원화하고 사유 메시지와 표시 시간을 TitleLevel로 전달
- `Timer::Set()`이 호출 시 `Reset()`을 함께 수행하도록 보완하여 Title 안내 메시지 표시 시간(약 3초) 정확히 보장
- Multiplayer 도중 서버 연결 종료 감지 시 네트워크 정리 후 Title로 복귀하여 `서버와의 연결이 끊어졌습니다.` 안내 메시지 출력
- Multiplayer 도중 local Player 사망(`_myPlayer->IsDead()`) 감지 시 `플레이어가 사망하였습니다.` 안내 메시지와 함께 Title로 안전하게 복귀

## 확인한 동작

- Enter packet의 정상·header 분할·payload 분할 수신
- Enter와 Input의 연속 전송 및 이동 뒤 `None` 입력으로 정지
- size `0`, `3`, `4097`과 잘못된 protocol version의 연결 거부
- 두 dummy client의 서로 다른 playerId와 Snapshot player count 변화
- 실제 Z1과 Z1Server의 connect → Enter → Snapshot → HUD 반영
- 실제 Z1 방향키의 sequence 증가와 서버 좌표 변화
- 실제 Z1 두 개에서 상대 playerId의 `NetworkPlayer` 생성과 Snapshot 위치 반영
- 실제 Z1 하나와 KeepAlive dummy client에서 local `MyPlayer`와 원격 `NetworkPlayer` 생성, local 입력 전송과 서버 좌표 반영 확인
- 같은 PC에서 실제 Z1 클라이언트 두 개와 Z1Server를 실행했을 때, 현재 포커스된 콘솔 창에만 키보드 입력이 전달되어 각 세션의 `MyPlayer`가 독립적으로 조작되는 것 확인
- Title에서 위/아래 키로 `LocalPlay`와 `MultiPlay` 순환 선택 및 Enter 시작 확인
- Local Play와 Multiplayer가 서로 간섭 없이 독립적으로 정상 동작하는 것 확인
- 서버 미실행 시 Multiplayer 선택 시 Title에서 약 3초간 "서버에 연결할 수 없습니다." 안내 메시지 출력 및 메뉴 재선택 가능 확인
- Multiplayer 플레이 중 서버 종료 시 네트워크 자원 정리 후 Title로 복귀하여 `서버와의 연결이 끊어졌습니다.` 안내 메시지 출력 확인
- Multiplayer 플레이 중 Player HP 0 사망 시 Title로 복귀하여 `플레이어가 사망하였습니다.` 안내 메시지 출력 및 서버 세션 종료 처리 확인
- 실제 Z1에서 BlockingMap 벽에 Player Box가 닿으면 서버 좌표가 더 이상 갱신되지 않음
- Z1Server와 Z1 `Debug|x64` 빌드 성공
- Enemy가 있는 Overworld Room에서 서버 Snapshot 기반 `NetworkEnemy` Sprite 표시 확인
- Moblin이 active Room의 Player를 대상으로 BlockingMap 장애물을 우회하는 A* 이동과 Snapshot 위치 갱신 확인
- Moblin만 Spear를 생성하며 Spear가 Snapshot 기반 `NetworkProjectile`로 표시되는 것 확인
- Spear가 blocked tile, `homeRoom` 경계 또는 lifetime 40 Tick에 도달하면 제거되는 것 확인
- Spear가 Player Box에 닿으면 HP가 1 감소해 HUD에 반영되고, HP 0에서 입력이 중지되며 Enemy가 해당 Player를 더 이상 추적하지 않는 것 확인
- 정지 상태에서 A 입력 한 번이 공격 요청으로 한 번만 소비되고, 이동 중 A 입력은 일반 검 공격으로 처리되지 않는 것 확인
- Player facing 앞의 Sword AABB와 겹친 같은 Room Enemy 한 명만 사망하며 범위 밖·반대 방향 Enemy는 영향을 받지 않는 것 확인
- 사망 Enemy가 즉시 이동·Projectile 발사를 중단하고 두 클라이언트의 Snapshot 표현에서 함께 제거되는 것 확인
- Enemy가 사망한 Room을 나갔다 다시 들어와도 해당 Enemy가 재등장하지 않는 것 확인
- 방향별 일반 검 Sprite 위치와 0.5초 뒤 제거, 빗나간 공격을 포함한 공격 효과음 1회 재생 확인
- 같은 Room의 실제 Z1 두 개에서 한 Player의 일반 검 CombatEvent가 양쪽에 전달되어 해당 NetworkPlayer 위치에 표현되는 것 확인
- 실제 Z1에서 서버 Moblin이 장애물을 우회할 때 `S2C_EnemyPathDebug` 경로가 이동 경로와 일치하게 표시되고, `F3` 토글로 표시를 전환하는 것 확인
- Enemy 사망 또는 Player의 Room 이동 시 이전 경로가 사라지고, ASCII 디버그 Sprite 캐싱 후 타일 밀림 없이 렌더링되는 것 확인
- peer disconnect와 protocol 오류가 `CloseSession()`의 단일 종료 전이로 수렴하고, pending completion drain 뒤 registry sweep 대상이 되는 것 확인
- 이동 중 공격 차단, 단일 입력의 1회 공격과 빠른 연속 입력 횟수만큼의 공격·표현 발생 확인
- CombatEvent wire format을 포함한 server framing suite와 Z1Server·Z1 빌드 통과

재현 명령은 [Z1 검증](TESTING.md)에 기록한다.

## 알려진 제약

- 서버는 Player의 Room을 영구 상태로 보관하지 않는다. 수신 Player의 서버 좌표에서 관심 Room을 계산하고, 그 Room의 Player·Enemy만 Snapshot에 담는다.
- 접근 가능한 Room 경로 whitelist는 서버에 아직 없다. BlockingMap과 전체 맵 경계로 도달 가능한 모든 Room을 이동할 수 있다.
- 클라이언트 네트워크 Room 전환은 Snapshot Player 위치의 기준점으로 판정한다. 싱글플레이의 leading-edge 전환 규칙과 통일하는 작업은 남아 있다.
- A* 이동과 최종 경로 시각화는 현재 Moblin에만 적용된다. 목표와 경로는 이동 기회마다 다시 계산하며, 서버 AI의 target/path cache는 아직 두지 않는다. 클라이언트 경로 map은 Enemy ID별 최신값을 보관하고 현재 Room만 렌더링한다. Octorok과 Tektite는 정적 상태다.
- Enemy 경로 디버그는 현재 모든 경로에 공통 표시 Sprite를 사용하며 Enemy별 문자·색상 구분은 후속 개선 사항이다.
- Enemy의 `homeRoom` 후보 검사는 좌상단 좌표 기준이다. `CanPlaceEnemy()`는 전체 8×5 Box를 검사하지만, Box 전체의 Room 경계 검사는 강화 대상이다.
- Player-Enemy 몸체 충돌과 접촉 피해는 아직 없다.
- Enemy 스폰은 BlockingMap의 전체 8×5 Box 통과 가능 여부만 검사한다. 같은 Room 안의 Enemy 위치 중복 방지는 아직 없다.
- 일반 검 표현은 `S2C_CombatEvent`의 단발 이벤트로 처리하므로 현재 `PlayerStateAttacking`과 `EnemyStateAttacking` flag는 사용하지 않는다. 지속 animation이나 방어 상태처럼 Snapshot에 유지할 공격 상태가 생길 때 존치 여부를 다시 결정한다.
- 최대 HP SwordBeam은 서버 Projectile과 protocol `ProjectileKind`에 아직 없다. 현재 서버 일반 검 판정은 Player HP와 관계없이 동일하게 적용된다.
- 일반 검의 좌우 10×3·상하 6×5 범위는 싱글플레이 Sword Sprite 크기를 사용한다. 시각 표현과 실제 판정 범위의 체감이 맞지 않으면 이 상수만 조정한다.
- Moblin Projectile은 현재 Spear 한 종류, 고정 피해량 1, lifetime 40 Tick이며 별도 공격 상태나 animation flag는 없다.
- Projectile은 현재 이동 후보 위치에서 Player 충돌을 검사한다. 방패, 무적 시간, 넉백과 Player별 피격 cooldown은 아직 없다.
- 현재 `NetworkClient::Start()`는 main thread에서 동기 `connect()`를 호출하므로, 서버 미실행 상태에서 Multiplayer 선택 시 blocking 호출이 반환될 때까지 Title 입력과 렌더링이 일시적으로 멈춘다. 현재 환경에서는 약 2초가 관찰됐지만 고정된 제한 시간은 아니다. 자세한 원인과 논블로킹 전환 보류 근거는 [네트워크 라이브러리 확장 검토 메모](NETWORK_LIBRARY_FOLLOWUPS.md#보류한-클라이언트-논블로킹-연결)에 기록한다.
- Multiplayer에서 Cave·Dungeon 입구 진입 시 미지원 안내 메시지 처리는 현재 보류되었다. 로컬 플레이와 달리 현재 서버에서는 Overworld 전체 Room(16×8) 접근이 가능하여 던전 입구가 여러 좌표에 분산되어 있으므로, 전체 입구 좌표 조사 및 안내 처리는 향후 시간 여유가 생겼을 때 다시 시도할 항목으로 남긴다.
- 런타임 중 닫힌 Session의 closing 전이, pending Recv/Send completion drain과 `_sessions` registry 제거가 적용됐다. `Server::Stop()`에서 모든 Session을 닫고 IOCP를 drain하는 전체 프로세스 종료 안정화는 별도 후속 작업이다.
- `S2C_Disconnect`는 선언만 되어 있다.
- 자동 재접속과 Session 복구는 지원하지 않는다.

## 다음 구현 단위

재구성한 전체 순서와 단계별 완료 조건은 [멀티플레이 잔여 작업 로드맵](MULTIPLAYER_ROADMAP.md)을 따른다. A* 최종 경로 시각화, 런타임 중 닫힌 Session registry 제거, 동일 PC 멀티클라이언트 입력 분리, Local/Multiplayer 모드 분리와 fallback 제거, Player 사망 후 Title 복귀까지 완료됐다. 바로 다음 작업은 Enemy 7초 리스폰이다.

그 뒤 서버 프로세스 종료 안정화 순으로 진행한다. SwordBeam·방패·무적·넉백과 Octorok·Tektite 고유 AI는 필수 범위가 끝난 뒤 시간이 남을 때만 재검토한다. Enemy·IOCP 확장 후보의 보류 근거는 [네트워크 라이브러리 확장 검토 메모](NETWORK_LIBRARY_FOLLOWUPS.md)에 기록한다.
