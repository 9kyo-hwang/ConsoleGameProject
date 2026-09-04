# Z1 멀티플레이 잔여 작업 로드맵

마지막 갱신: 2026-09-04

## 문서 목적

이 문서는 현재 멀티플레이 vertical slice 이후의 잔여 작업을 우선순위와 완료 조건으로 정리한다. 확정된 장기 계약은 [멀티플레이 설계](MULTIPLAYER_DESIGN.md), 구현·검증된 현재 상태는 [멀티플레이 현황](MULTIPLAYER_STATUS.md), 실행 검증은 [Z1 빌드와 검증](TESTING.md)을 기준으로 한다.

로드맵의 순서는 다음 작업을 고르기 위한 기준이며, 완료되지 않은 항목을 현재 구현처럼 간주하지 않는다. 한 번에 한 단계의 최소 기능만 구현하고 빌드·자동 검사·필요한 수동 검증을 통과한 뒤 다음 단계로 이동한다.

## 재구성한 MVP 완료 조건

다음 흐름이 실제 Z1 클라이언트 두 개에서 안정적으로 동작하면 현재 멀티플레이 MVP를 완료한 것으로 본다.

```text
Title에서 Local/Multiplayer 선택
→ 서로 다른 Player로 접속
→ 독립 입력으로 Overworld 이동·일반 검 공격
→ 서버 Moblin의 실제 A* 추적 경로 표시
→ Enemy 사망과 7초 뒤 리스폰 공유
→ Player 사망 또는 연결 종료
→ 네트워크 상태 정리 후 Title 복귀
→ 새 Player로 재접속
```

이번 완료 조건에는 최대 HP SwordBeam, 방패, 피격 무적·넉백과 Octorok·Tektite의 고유 AI를 포함하지 않는다. 기본 흐름이 안정된 뒤 시간이 남을 때만 선택적으로 추가한다.

## 확정 정책

### Enemy 리스폰

- 모든 Enemy는 사망 후 약 7초, 즉 20Hz 기준 140 Tick 뒤 같은 `networkId`와 최초 `spawnPosition`으로 부활한다.
- Enemy마다 별도 실시간 Timer 객체를 두지 않고 서버 Tick 기준의 `respawnAtTick`을 보관한다.
- Room에 Player가 없어 Enemy AI가 멈춘 동안에도 리스폰 시간은 흐른다.
- 리스폰 시간이 됐을 때 살아 있는 Player Box가 최초 스폰 Box와 실제로 겹치면 부활을 연기하고 다음 Tick에 다시 검사한다. 다른 임의 위치는 찾지 않는다.
- 죽은 Player는 스폰 겹침 검사에서 제외한다. 기존 정책과 마찬가지로 Enemy끼리의 위치 중복은 이번 단계에서 새로 해결하지 않는다.
- 리스폰하면 HP, 위치, 방향, 이동·공격 cooldown과 임시 AI 상태를 초기값으로 되돌린다.
- dead Enemy는 지금처럼 일반 Snapshot에서 빠지고, 리스폰 뒤 다시 포함된다. 별도 Spawn/Respawn packet은 추가하지 않는다.
- 서버를 재시작하면 모든 Enemy는 최초 상태로 시작한다.

Enemy 사망 시 이미 발사된 소유 Projectile을 즉시 제거할지는 리스폰 구현 전에 한 번 결정한다. 최소안은 사망한 Enemy가 소유한 Projectile도 함께 제거하는 것이다.

### Player 사망과 재접속

- 멀티플레이에서 한 Player의 사망은 다른 Player의 게임을 끝내지 않는다.
- local Player의 dead Snapshot을 처음 확인하면 클라이언트 연결을 닫고 Title로 바로 복귀한다. 서버는 peer close를 통해 해당 Session과 Player를 정리한다.
- 별도 Multiplayer GameOver Level이나 Overworld 사망 대기 상태는 우선 만들지 않는다.
- Title 메뉴 아래에 `플레이어가 사망하였습니다.`와 같은 짧은 메시지를 약 3초간 표시한다. 연결 실패와 연결 종료도 같은 메시지 전달 구조를 사용한다.
- 사망한 사용자가 Multiplayer에 다시 들어오면 새 `playerId`, 최대 HP와 시작 Room을 가진 새 Player로 입장한다.
- Enemy의 현재 위치·사망·리스폰 상태는 서버 월드에 남으므로 해당 사용자의 재접속 때문에 초기화하지 않는다.

## 작업 순서

### 1. 서버 A* 최종 경로 시각화 (완료)

외부 과제 요구사항이므로 다른 선택 전투 기능보다 먼저 완료한다.

구현 범위:

- 서버가 Moblin 이동에 실제 사용한 `RoomPathfinder::FindPath()` 결과를 클라이언트에 전달한다.
- 현재 관심 Room의 모든 Moblin 경로를 전달한다.
- 클라이언트는 경로를 재계산하지 않고 서버가 보낸 최종 경로만 표시한다.
- Moblin이 경로를 다시 계산할 때마다 해당 Enemy의 최신 표시 경로를 교체한다.
- 추적 대상이 없거나 경로 탐색 실패·Enemy 사망·Room 이탈 시 이전 표시를 제거한다.
- `F3` 로컬 디버그 키로 표시를 켜고 끈다. 입력이 꺼져 있어도 게임 판정과 서버 AI는 변하지 않는다.
- 여러 Moblin 경로를 모두 순회해 표시한다. 현재는 공통 디버그 표시 Sprite를 사용한다.

`WorldSnapshot`과 분리한 `S2C_EnemyPathDebug` packet으로 한 Moblin의 `tick`, `enemyId`, `homeRoom`, 경로 node 수와 16×11 Room의 tile index 배열을 보낸다. tile index는 `tileY * 16 + tileX`의 0~175 값이라 1 byte로 표현할 수 있다. 현재는 protocol version을 올리지 않고 v4 packet 목록에 추가했다.

완료 조건:

- 실제 Moblin이 장애물을 우회할 때 표시 경로와 이동이 일치한다.
- 같은 Room의 모든 Moblin 경로가 표시되고 갱신된다.
- 경로가 사라져야 하는 조건에서 오래된 선이 남지 않는다.
- 표시 On/Off가 AI, Snapshot과 일반 입력에 영향을 주지 않는다.

완료 검증:

- 서버가 실제 A* 계산 결과를 `S2C_EnemyPathDebug`로 전송하고 클라이언트가 Enemy ID별 최신 경로를 표시한다.
- `F3` 입력으로 경로 표시를 켜고 끌 수 있다.
- Enemy 사망과 Player의 Room 이동에서 이전 경로가 사라진다.
- 디버그 Sprite를 캐싱하고 ASCII 표시 문자를 사용해 타일이 밀려 보이는 렌더링 오류가 없다.

### 2. 닫힌 Session 제거 (완료)

transport 종료와 protocol 오류를 closing 상태로 한 번만 전환하고, outstanding I/O completion이 정리된 뒤에만 Session을 제거한다. `_sessions`는 IO thread가 안전한 지점에서 sweep하며, Session 객체를 IOCP completion이 남아 있는 동안 파괴하지 않는다.

구현 범위:

- `CloseSession()`이 `_closing` 전환, `RemovePlayer()`와 socket close를 한 번만 수행한다.
- `PostRecv()`와 `PostSend()`가 outstanding I/O를 pending으로 기록하고, IOLoop이 Recv/Send `OVERLAPPED` completion을 확인할 때 이를 해제한다. 성공·실패·취소 completion 모두 같은 규칙을 따른다.
- closing Session은 Broadcast와 다음 I/O 등록 대상에서 제외한다.
- `RemoveClosedSessions()`가 IOLoop 반복문 시작의 안전한 지점에서 `closing && no pending I/O` Session만 `_sessions`에서 제거한다.

- `OverworldSimulation::RemovePlayer()`를 정확히 한 번 호출한다.
- 다른 Session의 다음 Snapshot에서 종료된 Player가 사라진다.
- 반복 접속·종료에도 Session과 Player가 누적되지 않는다.

완료 검증:

- peer disconnect, malformed packet, I/O failure와 SendQueue 상한 초과가 같은 idempotent 종료 경로로 수렴한다.
- 취소된 Recv/Send completion을 소비하기 전에는 Session이 registry에 남고, 양쪽 pending이 모두 해제된 뒤에만 제거된다.
- closing Session은 이후 Snapshot·CombatEvent·EnemyPathDebug Broadcast에서 건너뛴다.

### 3. 동일 PC 멀티클라이언트 입력 분리 (완료)

이후 모든 다중 클라이언트 검증을 안정화하기 위한 작업이다. 세부 후보와 위험은 [콘솔 입력 시스템 개선 검토](CONSOLE_INPUT_DESIGN.md)를 따른다.

구현 범위:

- `MyPlayer`만 입력을 읽는 현재 책임은 유지한다.
- `CraftEngine::Input`의 공개 `GetKey`/`GetKeyDown`/`GetKeyUp` API를 유지한다.
- 우선 후보인 console input buffer의 `KEY_EVENT_RECORD` prototype으로 각 console에 전달된 입력만 소비한다.
- focus 전환이나 key release 때 이전 방향의 `None` 전송이 누락되지 않게 한다.
- Z1에만 맞춘 영구적인 테스트용 입력 우회나 전역 polling fallback은 추가하지 않는다.

완료 조건:

- 같은 PC의 Z1 두 개 중 입력 대상인 client의 Session만 이동한다.
- key-down, key-up, held와 빠른 tap이 기존 의미를 유지한다.
- Z1, ShootingGame과 SokobanGame의 기존 입력을 회귀 검증한다.

완료 검증:

- `CraftEngine::Input`이 `STD_INPUT_HANDLE`의 `KEY_EVENT_RECORD` 및 `FOCUS_EVENT`를 drain하여 포커스된 콘솔 창의 입력만 소비한다.
- `KeyState`의 `held`, `pressed`, `released` 분리로 키 auto-repeat 시 `GetKeyDown` 중복 방지 및 1프레임 탭 엣지를 보존한다.
- `FOCUS_EVENT`로 포커스 이탈 시 모든 키가 해제되어 이동 멈춤(`None`)이 보장된다.
- 같은 PC에서 실제 Z1 두 개를 실행하고 각 콘솔 창에 입력했을 때, 포커스된 창의 세션만 `C2S_Input`이 갱신되어 각 캐릭터가 독립적으로 이동한다.
- Title 레벨 `VK_RETURN` 입력 및 인게임 조작이 정상 동작한다.

### 4. Local/Multiplayer 모드 분리와 fallback 제거 (완료)

[멀티플레이 설계의 플레이 모드와 연결 종료 정책](MULTIPLAYER_DESIGN.md#플레이-모드와-연결-종료-정책)을 구현한다.

- Title에 `LocalPlay`, `MultiPlay` 위·아래 메뉴를 표시하고 기본 선택은 `LocalPlay`로 둔다.
- Local Play는 NetworkClient를 시작하지 않는다.
- Multiplayer는 연결과 `S2C_Enter` 완료 뒤에만 Network Overworld로 진입한다.
- 별도 Loading Level 없이 Title에 `Connecting to server...`를 표시한다.
- 기존 서버 연결 종료 후 offline fallback과 네트워크 Actor를 로컬 Actor로 바꾸는 보정 코드를 제거한다.
- 연결 실패·종료는 한 네트워크 정리 경로를 거쳐 Title로 돌아가고 약 3초간 원인별 메시지를 표시한다.

완료 조건:

- 서버 실행 여부와 관계없이 Local Play를 정상 시작할 수 있다.
- 연결 실패·서버 종료 뒤 로컬 Player가 중복 생성되지 않는다.
- Title에서 실패 메시지를 보는 동안에도 다시 메뉴를 조작할 수 있다.

완료 검증:

- Title에서 `LocalPlay`와 `MultiPlay`를 위/아래 키로 선택하고 Enter로 시작할 수 있다.
- Local Play는 네트워크 초기화 없이 싱글플레이 `OverworldLevel`을 단독으로 시작한다.
- Multiplayer는 전용 `NetworkOverworldLevel`로 분리되었으며, 기존 `OverworldLevel`의 네트워크 Actor 및 offline fallback 잔재 코드가 전면 제거됐다.
- 서버 미실행 시 Multiplayer 선택 시 약 3초간 Title에 실패 안내 메시지가 표시되며 메뉴 조작이 유지된다.
- 인게임 중 서버 종료 시 `Game::OnDisconnect`를 통해 Title로 복귀하고 안내 메시지가 표시된다.

후속 보류: Multiplayer의 Cave·Dungeon 입구 진입 시 미지원 안내 표시는 이번 완료 범위에서 제외했다. 로컬 플레이와 달리 현재 서버에서는 Overworld 전체 Room(16×8) 접근이 가능하여 던전 입구가 여러 좌표에 분산되어 있으므로, 전체 입구 좌표 조사 및 안내 표시는 향후 시간 여유가 생겼을 때 다시 검토한다.

### 5. Player 사망 후 Title 복귀 (완료)

4단계의 공통 네트워크 정리와 Title 메시지 전달 경로를 재사용한다.

- local Player의 첫 dead 전환만 처리한다.
- 사망 효과음이 중복 재생되지 않고 입력은 지금처럼 차단된다.
- client socket을 닫고 Title로 전환한다. 서버는 peer close를 감지해 Session과 Player를 제거한다.
- 다른 클라이언트에서는 해당 Player가 제거되고 플레이를 계속할 수 있다.
- 다시 Multiplayer에 들어가면 시작 Room의 새 Player로 생성된다.

완료 조건:

- Multiplayer 중 HP 0 사망 시 네트워크 상태 정리 후 Title로 복귀한다.
- Title에서 `플레이어가 사망하였습니다.` 메시지가 표시되며 재시작 메뉴를 조작할 수 있다.
- 재접속 시 새 Player로 정상 참여할 수 있다.

완료 검증:

- `NetworkOverworldLevel::Tick`에서 local `MyPlayer`의 사망(`IsDead()`) 감지 시 `Game::OnDisconnect("플레이어가 사망하였습니다.")`를 호출한다.
- `NetworkClient::Stop()`이 socket, 양방향 queue, pending 전송 packet, framer, partial-send offset과 input sequence를 초기화하고 Title 화면으로 복귀하여 3초간 사망 안내 메시지를 출력한다.
- 서버에서는 플레이어가 제거되어 다른 클라이언트의 Snapshot에서 사라지며, Title에서 다시 Multiplayer를 선택하면 새 세션으로 정상 접속할 수 있다.

### 6. Enemy 7초 리스폰

확정된 Enemy 리스폰 정책을 서버에 추가한다.

- `Enemy`는 `respawnAtTick`과 초기 상태 복원 API만 추가한다.
- Simulation은 dead Enemy의 만료 여부를 active Room과 무관하게 확인한다.
- 최초 스폰 Box에 살아 있는 Player가 겹치면 위치가 비워질 때까지 부활을 미룬다.
- 부활 상태는 기존 WorldSnapshot 존재 여부로 모든 관심 클라이언트에 반영한다.

완료 조건:

- 여러 클라이언트에서 Enemy 사망과 약 7초 뒤 같은 위치의 재등장이 일치한다.
- Room을 비워도 리스폰 시간이 멈추지 않는다.
- 스폰 위치를 Player가 점유하는 동안 Enemy가 겹쳐 나타나지 않는다.
- 클라이언트 재접속은 Enemy의 리스폰 시간을 초기화하지 않는다.

### 7. 서버 프로세스 종료 안정화

- accept를 중지하고 accept thread를 join한다.
- 신규 I/O post를 막고 진행 중인 completion을 drain한다.
- server thread와 Session을 정리한 뒤 completion port를 닫는다.
- 정상 종료 중 use-after-free, 무한 대기와 중복 close가 없어야 한다.

### 8. 선택 전투 기능

필수 단계가 안정된 뒤 남은 시간에만 다음 순서로 추가한다.

1. Player–Enemy 접촉 피해
2. Octorok의 최소 이동과 Projectile 공격
3. Tektite의 타이머 기반 점프 이동

접촉 피해는 근접형 Enemy 동작의 공통 전제이므로 다른 Enemy AI보다 먼저 둔다. Octorok과 Tektite를 구현하더라도 범용 행동 트리나 서버 Pawn 계층을 먼저 만들지 않고 종류별 최소 Tick 규칙으로 시작한다.

## 보류 범위와 재검토 조건

다음 기능은 현재 MVP 완료 조건에서 제외한다.

- 최대 HP SwordBeam
- Player 방패와 Projectile 방어
- 피격 무적 시간
- 피격 넉백
- Octorok·Tektite의 원작 수준 세부 행동
- Cave·Dungeon·보스·아이템의 멀티플레이 동기화
- 자동 재접속과 진행 중 Session 복구

SwordBeam과 방패는 Player/Enemy Projectile 진영과 소유권 구분이 필요하다. 무적과 넉백은 피해 cooldown, 강제 이동과 서버 충돌 규칙을 추가한다. 현재 샌드박스 흐름이나 A* 과제 완료에 필수적이지 않으므로 앞 단계가 모두 끝나고 시간이 남을 때만 다시 범위를 정한다.

## 단계별 문서 갱신

각 단계가 끝날 때 다음만 수행한다.

1. [멀티플레이 현황](MULTIPLAYER_STATUS.md)에 구현·검증된 결과와 다음 한 단계를 기록한다.
2. wire 또는 확정 정책이 바뀌면 [멀티플레이 설계](MULTIPLAYER_DESIGN.md)를 함께 갱신한다.
3. 자동·수동 재현 방법을 [Z1 빌드와 검증](TESTING.md)에 추가한다.
4. 이 로드맵에서는 완료 표시와 다음 순서가 실제 우선순위와 다른 경우만 수정한다.
