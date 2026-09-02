# Z1 빌드와 검증

## 문서 범위

이 문서는 Z1, Z1Server와 Z1Shared 변경에 필요한 프로젝트 전용 검증을 정의한다. 전체 솔루션의 빌드 환경과 공통 검증 원칙은 [빌드와 개발](../DEVELOPMENT.md)을 따른다.

## 빌드

깨끗한 환경에서는 다음 순서로 `Debug|x64`를 빌드한다.

1. `SoundSystem`
2. `CraftEngine`
3. `Sockets`
4. `Z1Server`
5. `Z1`

싱글플레이 콘텐츠만 변경했다면 Z1 빌드가 최소 범위다. CraftEngine 공개 경계나 공용 런타임 동작을 변경했다면 ShootingGame과 SokobanGame도 회귀 빌드한다.

## 싱글플레이 기준선

### 시작과 오버월드

- 타이틀의 `Press Enter To Play` 뒤 `Enter`로 새 게임이 시작된다.
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

- `A` 검 공격은 이동 시 취소되고 공격 하나가 대상에 한 번만 피해를 준다.
- 최대 HP 검기와 적·보스 투사체가 발사자 Box 가장자리에서 생성된다.
- Octorok, Moblin, Tektite의 이동·공격과 Room 전환 시 Enemy/Projectile 제거를 확인한다.
- 투사체는 벽과 Room 경계를 통과하지 않고 Player 방패는 올바른 방향의 적 투사체를 막는다.
- 넉백은 벽을 통과하지 않으며 무적 시간에는 중복 피해가 없고 피격 Actor가 깜빡인다.
- 검 획득·공격·방어·피격·사망 효과음과 Title·Overworld·Dungeon·GameOver·Clear BGM 전환을 확인한다.
- 트라이포스 팬파레가 끝난 뒤 Clear의 Ending Theme으로 전환된다.

## 서버 packet 자동 검증

Z1Server를 먼저 실행한 뒤 PowerShell dummy client로 TCP framing과 기본 protocol을 확인한다.

```powershell
.\tools\test-z1-enter.ps1 -RunServerFramingSuite
```

이 suite는 protocol v3의 정상 Enter/Snapshot, header와 payload 분할, 연속 packet, Input 이동·정지, Enemy·Projectile 배열 파싱, 잘못된 packet size와 protocol version 거부를 확인한다. 기본 endpoint는 `127.0.0.1:7777`이다.

스크립트 통과는 실제 Z1 통합을 대체하지 않는다. transport나 Snapshot 표현을 변경했다면 Z1Server와 Z1을 함께 실행해 연결, playerId, 이동, 연결 종료 후 Actor 정리를 확인한다.

### KeepAlive dummy client

`MyPlayer`와 원격 표현을 수동으로 확인할 때는 Z1Server를 실행한 뒤 별도 PowerShell에서 다음 dummy client를 유지한다.

```powershell
.\tools\test-z1-enter.ps1 -KeepAlive
```

이후 실제 Z1 하나를 실행한다. dummy와 Z1이 서로 다른 playerId로 Snapshot에 나타나고, Z1의 local `MyPlayer` 입력·서버 이동·원격 `NetworkPlayer` 생성과 정지를 확인한다. dummy PowerShell에서 `Ctrl+C`를 누르면 연결이 종료되며, Z1에서 원격 표현 제거와 offline 복귀를 확인한다.

### 서버 Enemy와 Projectile 표현

Z1Server를 실행한 뒤 실제 Z1을 Enemy가 생성되는 Overworld Room으로 이동한다. 해당 Room의 Enemy Sprite가 `NetworkEnemy`로 표시되고, 다른 Room으로 이동하면 이전 Room의 표현이 제거되는지 확인한다. 다시 원래 Room에 들어가면 같은 server Enemy가 다시 표시되어야 한다.

Moblin이 같은 Room의 살아 있는 Player를 A*로 추적하고 Spear를 발사하는지 확인한다. Spear는 `NetworkProjectile`의 `-` 또는 `|`로 표시되어야 하며 blocked tile, Room 경계 또는 lifetime 40 Tick에서 사라져야 한다. Player와 겹치면 Spear가 사라지고 HP가 1 감소해야 한다. HP 0에서는 입력이 중지되고 Moblin이 해당 Player를 추적 대상으로 고르지 않아야 한다.

현재 Player와 Enemy 몸체는 서로를 막지 않으며 접촉 피해도 없다. 방패, 피격 무적과 넉백도 네트워크 모드에는 아직 없다. Enemy 스폰 위치는 8×5 Box 전체가 `BlockingMap`의 통행 가능 타일에 들어가는지로 확인한다. 같은 Room 내 위치 중복은 아직 허용되는 알려진 제약이다.

### 서버 Player 일반 검과 Enemy 사망

Enemy가 있는 Room에서 Player가 정지한 상태로 Enemy를 바라보고 A를 한 번 누른다. 일반 검은 현재 보이지 않지만 서버가 좌우 10×3·상하 6×5 AABB로 같은 Room의 살아 있는 Enemy를 검사한다.

- 한 번의 A 입력이 한 번만 소비되고 겹친 Enemy 중 network ID가 가장 작은 한 명만 피해를 받는다.
- 이동 방향키를 누른 상태에서 A를 눌러도 일반 검 공격이 실행되지 않는다. 방향키를 놓은 뒤 이전 공격 요청이 뒤늦게 실행되어서도 안 된다.
- facing 앞의 범위 안 Enemy만 사망하고 범위 밖이나 반대 방향 Enemy는 유지된다.
- 사망 Enemy는 즉시 이동과 Projectile 생성을 중단하고, 같은 Room을 보는 모든 Z1 클라이언트에서 제거된다.
- Room을 나갔다 돌아와도 사망 Enemy는 다시 나타나지 않는다.

현재 모든 Enemy HP는 1이고 일반 검 피해도 1이므로 피격과 사망이 동시에 일어난다. 검 Sprite·공격 효과음, 최대 HP SwordBeam과 Player/Enemy 공격 상태 flag 표현은 이 검증 범위에 포함하지 않는다.

### 서버 종료 후 offline fallback — 폐기 예정인 현재 동작

1. Z1Server와 Z1을 실행하고 시작 Room이 아닌 Enemy Room으로 이동한다.
2. `MyPlayer`, `NetworkEnemy`와 필요하면 KeepAlive dummy의 `NetworkPlayer`가 표시되는 것을 확인한다.
3. Z1Server를 종료하고 HUD가 `[OFFLINE]`으로 바뀐 뒤 방향키를 입력한다.

현재 재현 결과는 실패다. 기존 네트워크 Player가 제거되지 않고 오프라인 `Player`가 추가로 보이며, 새 `Player`에는 키보드 입력이 반영되지 않는다. 네트워크 표현과 Room의 로컬 Enemy가 함께 남아 보일 수도 있다. fallback 위치 전달, `_wasOnline` 전환 flag와 비활성 `_player` 정리를 적용한 상태에서도 같은 증상이 재현된다.

이 fallback은 더 이상 목표 동작이 아니므로 성공시키기 위한 추가 보정은 하지 않는다. 향후 플레이 모드 분리 전까지는 현재 알려진 오류로 남긴다.

### 향후 플레이 모드와 연결 종료 검증

플레이 모드 분리를 구현할 때 다음을 확인한다.

- Title은 `Local Play`, `Multiplayer` 순서로 표시되고 기본 선택은 `Local Play`다. 위/아래 키로 순환 선택하고 Enter로 확정할 수 있다.
- Local Play 선택 시 서버가 실행 중이어도 연결하지 않고 기존 싱글플레이가 정상 동작한다.
- Multiplayer 선택 시 별도 Level 전환 없이 Title에 `Connecting...`을 표시하고, 연결과 Enter가 완료된 뒤에만 Network Overworld로 진입한다.
- 최초 연결 실패 시 로컬 게임을 시작하지 않고 Title 메뉴 아래에 약 3초간 실패 메시지를 표시한다.
- Multiplayer 도중 서버가 종료되면 네트워크 Player·Enemy·Projectile과 client 상태를 정리하고 Title로 돌아가 약 3초간 연결 종료 메시지를 표시한다.
- 메시지가 표시되는 동안에도 메뉴를 조작하고 Multiplayer 연결을 다시 시도할 수 있다.
- 종료 직전 서버 좌표, HP와 Enemy 상태가 새 Local Play에 승계되지 않는다.
- Title에서 다시 Local Play를 시작하거나 Multiplayer 연결을 재시도할 수 있다.
- Multiplayer에서는 Cave·Dungeon 입구에 닿아도 Level을 전환하지 않고 짧은 미지원 안내를 표시한다. 같은 입구가 Local Play에서는 기존대로 동작한다.

## 현재 멀티클라이언트 제한

같은 desktop에서 실행한 두 Z1 프로세스는 `GetAsyncKeyState` 때문에 같은 물리 키를 동시에 감지할 수 있다. 입력 방식이 개선되기 전에는 실제 Z1 하나와 dummy client 조합으로 독립 이동을 검증한다. 원인과 후보는 [콘솔 입력 설계](CONSOLE_INPUT_DESIGN.md)를 참고한다.

## 문서와 프로젝트 파일

Z1 문서 링크와 프로젝트 파일 등록은 저장소 공통 검사 명령을 사용한다.

```powershell
.\tools\Test-Documentation.ps1 -StrictChangeAudit
.\tools\Test-ProjectFiles.ps1
```
