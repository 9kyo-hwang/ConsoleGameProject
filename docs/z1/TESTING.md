# Z1 빌드와 검증

## 문서 범위

Z1, Z1Server와 Z1Shared 변경에 필요한 프로젝트 전용 검증을 서술합니다.

## 빌드

다음 순서로 빌드를 수행합니다.

1. `SoundSystem`
2. `CraftEngine`
3. `Sockets`
4. `Z1Server`
5. `Z1`

싱글플레이 콘텐츠만 변경했다면 Z1만 빌드합니다. 엔진단 코드를 변경했다면 ShootingGame과 SokobanGame도 [다시 빌드]합니다.

## 싱글플레이 기준선

### 시작과 오버월드

- 타이틀에서 `LocalPlay`를 `Enter`로 선택하면 새 싱글플레이 게임이 시작된다.
- 시작 Room `(7, 7)`에서 Player와 HUD의 HP·검 상태가 표시된다.
- 막힌 타일과 맵 밖으로 이동할 수 없고 Player Box 전체가 통행 가능 영역에 있을 때만 이동한다.
- `(7,7) → (7,6) → (8,6) → (8,5) → (8,4) → (8,3) → (7,3)` 경로의 Room을 왕복하고 허용하지 않은 경계를 넘지 않는다.
- Room 전환과 넉백 뒤에도 Player Box와 Sprite가 새 Room 안에 완전히 보인다.

### 동굴과 던전

- `(7, 7)`의 검 동굴에서 검 Sprite에 닿으면 검을 얻고 오버월드로 돌아올 수 있다.
- 검이 없으면 `(7, 3)`의 Dungeon 1 입구에 들어갈 수 없고, 검 획득 후에는 입구와 던전 출구가 정상 전환된다.
- 던전 Room 전환과 넉백 전환에도 경계 위치 보정이 적용된다.
- Aquamentus 처치 후 하트로 HP를 회복하고 트라이포스 획득 뒤 Clear로 전환한다.
- Player HP가 0이면 GameOver로 전환하고 입력으로 Title에 돌아온다.

### 전투와 사운드

- `A`키 입력으로 수행하는 검 공격은 이동하지 않을 때만 발동하고, 공격 한 번 당 대상에게 한 번만 피해를 입힌다.
- 최대 HP 시 발동하는 SwordBeam과 적·보스 투사체는 발사 주체의 Box 콜라이더 가장자리에서 생성된다.
- Octorok, Moblin, Tektite의 이동·공격과 Room 전환 시 Enemy/Projectile 제거를 확인한다.
- 투사체는 벽과 Room 경계를 통과하지 않고 Player 방패는 올바른 방향의 적 투사체를 막는다.
- 넉백은 벽을 통과하지 않으며 무적 시간에는 중복 피해가 없고 피격 Actor가 깜빡인다.
- 검 획득·공격·방어·피격·사망 효과음과 Title·Overworld·Dungeon·GameOver·Clear BGM 전환을 확인한다.
- 트라이포스 획득 시 재생되는 BGM이 끝난 뒤 Clear 레벨의 Ending Theme으로 전환된다.

## 서버 packet 자동 검증

Z1Server를 먼저 실행한 뒤 PowerShell dummy client로 TCP framing과 기본 protocol을 확인한다.

```powershell
.\tools\test-z1-enter.ps1 -RunServerFramingSuite
```

이 suite는 protocol v4의 정상 Enter/Snapshot, header와 payload 분할, 연속 packet, Input 이동·정지, Enemy·Projectile 배열 파싱, 일반 검 입력에 대한 CombatEvent의 크기·type·actorId·방향, 잘못된 packet size와 protocol version 거부를 확인한다. 기본 endpoint는 `127.0.0.1:7777`이다. Snapshot을 읽는 경로는 `S2C_EnemyPathDebug`도 함께 파싱하므로 두 packet이 섞여도 framing 검사가 중단되지 않는다.

실제 경로 packet을 반드시 확인하려면 Moblin이 있는 Room으로 이동할 수 있는 입력 방향과 유지 시간을 지정해 `-ExpectEnemyPathDebug`를 추가한다. 이 옵션은 최대 200개의 서버 packet을 읽으면서 `tick`, Enemy ID, Room 좌표, node count, tile index 범위와 payload 길이를 검증한다. 시작 Room `(7,7)`에는 Enemy가 없으므로 해당 Room에 머문 상태에서는 이 옵션을 사용하지 않는다.

스크립트 통과는 실제 Z1 통합을 대체하지 않는다. transport나 Snapshot 표현을 변경했다면 Z1Server와 Z1을 함께 실행해 연결, playerId, 이동, 연결 종료 후 Actor 정리를 확인한다.

### Session 종료와 registry 정리

`tools/test-z1-enter.ps1 -RunServerFramingSuite`를 반복 실행하거나 dummy client를 연결한 뒤 종료한다. Visual Studio debugger 또는 서버 로그에서 다음을 확인한다.

- peer disconnect, malformed packet과 I/O failure가 `CloseSession()`으로 수렴한다.
- 같은 Session에서 `_closing` 전환, `RemovePlayer()`와 socket close가 각각 한 번만 발생한다.
- 취소된 Recv/Send completion이 도착하기 전에는 `_sessions`의 Session이 유지된다.
- `_recvPending`과 `_sendPending`이 모두 해제된 다음 IOLoop 반복 시작에서 `RemoveClosedSessions()`가 해당 Session을 제거한다.
- 반복 접속·종료 후 `_sessions`와 Simulation의 Player 수가 누적되지 않는다.

이 검증은 런타임 연결 종료 정리 범위다. `Server::Stop()`에서 모든 Session을 닫고 IOCP completion을 drain하는 프로세스 전체 종료 검증은 별도 후속 작업이다.

### KeepAlive dummy client

`MyPlayer`와 원격 표현을 수동으로 확인할 때는 Z1Server를 실행한 뒤 별도 PowerShell에서 다음 dummy client를 유지한다.

```powershell
.\tools\test-z1-enter.ps1 -KeepAlive
```

이후 실제 Z1 하나를 실행한다. dummy와 Z1이 서로 다른 playerId로 Snapshot에 나타나고, Z1의 local `MyPlayer` 입력·서버 이동·원격 `NetworkPlayer` 생성과 정지를 확인한다. dummy PowerShell에서 `Ctrl+C`를 누르면 dummy 연결만 종료되며, 실제 Z1은 online 상태를 유지하고 dummy에 대응하는 원격 `NetworkPlayer` 표현만 제거해야 한다.

### 서버 Enemy와 Projectile 표현

Z1Server를 실행한 뒤 실제 Z1을 Enemy가 생성되는 Overworld Room으로 이동한다. 해당 Room의 Enemy Sprite가 `NetworkEnemy`로 표시되고, 다른 Room으로 이동하면 이전 Room의 표현이 제거되는지 확인한다. 다시 원래 Room에 들어가면 같은 server Enemy가 다시 표시되어야 한다.

Moblin이 같은 Room의 살아 있는 Player를 A*로 추적하고 Spear를 발사하는지 확인한다. Spear는 `NetworkProjectile`의 `-` 또는 `|`로 표시되어야 하며 blocked tile, Room 경계 또는 lifetime 40 Tick에서 사라져야 한다. Player와 겹치면 Spear가 사라지고 HP가 1 감소해야 한다. HP 0에서는 입력이 중지되고 Moblin이 해당 Player를 추적 대상으로 고르지 않아야 한다.

현재 Player와 Enemy 몸체는 서로를 막지 않으며 접촉 피해도 없다. 방패, 피격 무적과 넉백도 네트워크 모드에는 아직 없다. Enemy 스폰 위치는 8×5 Box 전체가 `BlockingMap`의 통행 가능 타일에 들어가는지로 확인한다. 같은 Room 내 위치 중복은 아직 허용되는 알려진 제약이다.

### Moblin A* 경로 디버그 표시

Z1Server와 Z1을 실행하고 Moblin이 생성되는 Overworld Room으로 이동한다. `F3`를 눌러 경로 표시를 켜면 서버가 계산한 현재 경로가 Room 타일 위에 표시되어야 하며, 장애물을 우회하는 표시 순서가 Moblin의 실제 이동과 일치해야 한다. 다시 `F3`를 누르면 표시가 사라지고 AI·Snapshot·일반 입력은 계속 동작해야 한다.

- 같은 Room에 여러 Moblin이 있으면 수신된 각 Enemy ID의 경로가 표시되고 새 경로 계산 때 갱신된다.
- 경로 탐색 실패 또는 추적 대상 상실의 빈 경로에서는 이전 표시가 남지 않는다.
- Moblin 사망 또는 Player의 Room 이동 뒤 이전 Room의 경로가 현재 화면에 남지 않는다.
- 디버그 표시가 타일 경계를 밀어내거나 깨진 다중 바이트 문자로 보이지 않아야 한다.

### 서버 Player 일반 검과 Enemy 사망

Enemy가 있는 Room에서 Player가 정지한 상태로 Enemy를 바라보고 A를 한 번 누른다. 서버는 좌우 10×3·상하 6×5 AABB로 같은 Room의 살아 있는 Enemy를 검사하고, 승인된 공격을 CombatEvent로 같은 Room의 클라이언트에 전송한다.

- 한 번의 A 입력이 한 번만 소비되고 겹친 Enemy 중 network ID가 가장 작은 한 명만 피해를 받는다.
- 이동 방향키를 누른 상태에서 A를 눌러도 일반 검 공격이 실행되지 않는다. 방향키를 놓은 뒤 이전 공격 요청이 뒤늦게 실행되어서도 안 된다.
- facing 앞의 범위 안 Enemy만 사망하고 범위 밖이나 반대 방향 Enemy는 유지된다.
- 사망 Enemy는 즉시 이동과 Projectile 생성을 중단하고, 같은 Room을 보는 모든 Z1 클라이언트에서 제거된다.
- Room을 나갔다 돌아와도 사망 Enemy는 다시 나타나지 않는다.
- 위·아래·왼쪽·오른쪽 공격마다 검 Sprite가 공격한 `MyPlayer` 또는 `NetworkPlayer`의 해당 방향에 표시된다.
- 공격이 빗나가도 검 Sprite와 `LOZ_Sword_Slash.wav`가 한 번 재생되고, 검 Sprite는 약 0.5초 뒤 제거된다.
- 빠르게 A를 여러 번 누르면 서버가 승인한 횟수만큼 표현되며, 한 번의 입력이 여러 Tick에서 반복 재생되지 않는다.
- 같은 Room에 실제 Z1 두 개를 접속하면 한쪽의 공격 표현과 효과음이 양쪽에 전달된다. 각 콘솔 창의 포커스에 따라 독립적으로 입력이 전달되므로 한쪽 창을 조작해 CombatEvent 전파를 확인한다.
- 클라이언트만 재시작해도 서버가 실행 중인 동안 사망 Enemy는 다시 생성되지 않는다. Enemy 초기 상태를 다시 확인하려면 서버를 재시작한다.

현재 모든 Enemy HP는 1이고 일반 검 피해도 1이므로 피격과 사망이 동시에 일어난다. 최대 HP SwordBeam과 Player/Enemy 공격 상태 flag 표현은 이 검증 범위에 포함하지 않는다.

### 플레이 모드 분리와 연결 종료·사망 복귀 검증

Local Play와 Multiplayer의 진입 분리 및 연결 종료·사망 시 Title 복귀 동작을 확인한다.

1. **모드 선택과 Local Play 독립 검증**:
   - Title에서 위/아래 키로 `LocalPlay`, `MultiPlay`를 순환 선택하고 Enter로 확정할 수 있는지 확인한다. 기본 선택은 `LocalPlay`다.
   - 서버가 꺼져 있거나 켜져 있는 상태에서 `LocalPlay`를 선택하면 `NetworkClient::Start()`를 호출하지 않고 싱글플레이 `OverworldLevel`로 즉시 시작되는지 확인한다.

2. **Multiplayer 연결 실패 및 Title 대기 검증**:
   - Z1Server가 실행되지 않은 상태에서 Title의 `MultiPlay`를 선택한다.
   - `connect()` 대기 후 Title 메뉴 아래에 `서버에 연결할 수 없습니다.` 메시지가 약 3초간 표시되는지 확인한다.
   - 메시지가 표시되는 동안에도 위/아래 키로 메뉴를 조작할 수 있고 `LocalPlay`를 바로 시작할 수 있는지 확인한다.

3. **Multiplayer 정상 연결과 서버 종료 복귀 검증**:
   - Z1Server를 실행한 상태에서 `MultiPlay`를 선택해 `Connecting to server...` 표시 후 `NetworkOverworldLevel`로 정상 진입하는지 확인한다.
   - 인게임 플레이 중 Z1Server 콘솔을 강제 종료(Ctrl+C 등)한다.
   - Z1 클라이언트가 소켓 연결 끊김을 감지하고 모든 네트워크 Actor를 정리한 뒤 Title로 복귀하며, `서버와의 연결이 끊어졌습니다.` 메시지가 약 3초간 표시되는지 확인한다.

4. **Multiplayer 플레이어 사망 시 Title 복귀 검증**:
   - Z1Server를 실행하고 `MultiPlay`로 접속하여 적 투사체(Spear)에 피격되어 Player HP가 0이 되도록 한다.
   - 사망 즉시 소켓 종료 및 정리 후 Title로 복귀하며, `플레이어가 사망하였습니다.` 메시지가 약 3초간 표시되는지 확인한다.
   - 서버에서 해당 세션이 정상 제거되어 다른 클라이언트의 Snapshot에서 사망 Player가 사라지는지 확인한다.
   - Title에서 다시 `MultiPlay`를 선택해 새 Player로 Overworld 시작 Room에 재접속할 수 있는지 확인한다.
   - 재접속 직후 첫 입력과 `C2S_Enter`가 정상 처리되고, 이전 세션의 미전송 input, partial packet, 수신 Snapshot·CombatEvent 또는 playerId가 나타나지 않는지 확인한다.
   - 서버 미실행 연결 실패를 한 번 이상 거친 뒤 서버를 실행하고 다시 `MultiPlay`를 선택해 새 socket으로 정상 접속되는지 확인한다.

5. **Cave·Dungeon 입구 처리 보류 사항**:
   - Multiplayer에서 Cave·Dungeon 입구 진입 시 미지원 안내 메시지 처리는 현재 보류되었다. 로컬 플레이와 달리 현재 서버에서는 Overworld 전체 Room(16×8) 접근이 가능하여 던전 입구가 여러 좌표에 분산되어 있으므로, 전체 입구 좌표 조사 및 안내 표시는 향후 시간 여유가 생겼을 때 다시 시도한다. (Local Play에서는 기존대로 동굴/던전 진입이 정상 동작한다.)

## 다중 클라이언트 독립 입력 검증

콘솔 입력 버퍼 기반 전환으로 동일 PC에서 실행한 여러 Z1 프로세스의 입력이 콘솔 창 포커스 단위로 분리된다.

1. Z1Server를 실행한 뒤 실제 Z1 프로세스 두 개를 실행하여 각각 접속한다.
2. 한쪽 콘솔 창을 활성화(포커스)하고 방향키를 입력했을 때, 해당 콘솔의 `MyPlayer`만 이동하고 다른 콘솔의 Player는 이동하지 않는지 확인한다.
3. 다른 쪽 콘솔 창을 클릭해 포커스를 옮긴 뒤 방향키를 입력했을 때, 새로 포커스된 콘솔의 `MyPlayer`만 독립적으로 이동하는지 확인한다.
4. 이동 방향키를 누른 상태에서 마우스로 다른 창을 클릭하거나 Alt+Tab으로 포커스를 잃었을 때, `FOCUS_EVENT`에 의해 키가 해제되어 캐릭터 이동이 멈추는지(`None` 전달) 확인한다.

## 문서와 프로젝트 파일

Z1 문서 링크와 프로젝트 파일 등록은 저장소 공통 검사 명령을 사용한다.

```powershell
.\tools\Test-Documentation.ps1 -StrictChangeAudit
.\tools\Test-ProjectFiles.ps1
```
