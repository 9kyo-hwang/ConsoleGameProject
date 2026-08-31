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

이 suite는 protocol v2의 정상 Enter/Snapshot, header와 payload 분할, 연속 packet, Input 이동·정지, Enemy 배열 파싱, 잘못된 packet size와 protocol version 거부를 확인한다. 기본 endpoint는 `127.0.0.1:7777`이다.

스크립트 통과는 실제 Z1 통합을 대체하지 않는다. transport나 Snapshot 표현을 변경했다면 Z1Server와 Z1을 함께 실행해 연결, playerId, 이동, 연결 종료 후 Actor 정리를 확인한다.

### KeepAlive dummy client

`MyPlayer`와 원격 표현을 수동으로 확인할 때는 Z1Server를 실행한 뒤 별도 PowerShell에서 다음 dummy client를 유지한다.

```powershell
.\tools\test-z1-enter.ps1 -KeepAlive
```

이후 실제 Z1 하나를 실행한다. dummy와 Z1이 서로 다른 playerId로 Snapshot에 나타나고, Z1의 local `MyPlayer` 입력·서버 이동·원격 `NetworkPlayer` 생성과 정지를 확인한다. dummy PowerShell에서 `Ctrl+C`를 누르면 연결이 종료되며, Z1에서 원격 표현 제거와 offline 복귀를 확인한다.

### 서버 Enemy 정적 표현

Z1Server를 실행한 뒤 실제 Z1을 Enemy가 생성되는 Overworld Room으로 이동한다. 해당 Room의 Enemy Sprite가 `NetworkEnemy`로 표시되고, 다른 Room으로 이동하면 이전 Room의 표현이 제거되는지 확인한다. 다시 원래 Room에 들어가면 같은 server Enemy가 다시 표시되어야 한다.

현재 Enemy는 스폰만 되며 이동·접촉·공격·피해를 처리하지 않는다. 따라서 이 단계에서는 Enemy를 통과해도 정상이다. 스폰 위치는 Enemy의 8×5 Box 전체가 `BlockingMap`의 통행 가능 타일에 들어가는지로 확인한다. 같은 Room 내 위치 중복은 아직 허용되는 알려진 제약이다.

## 현재 멀티클라이언트 제한

같은 desktop에서 실행한 두 Z1 프로세스는 `GetAsyncKeyState` 때문에 같은 물리 키를 동시에 감지할 수 있다. 입력 방식이 개선되기 전에는 실제 Z1 하나와 dummy client 조합으로 독립 이동을 검증한다. 원인과 후보는 [콘솔 입력 설계](CONSOLE_INPUT_DESIGN.md)를 참고한다.

## 문서와 프로젝트 파일

Z1 문서 링크와 프로젝트 파일 등록은 저장소 공통 검사 명령을 사용한다.

```powershell
.\tools\Test-Documentation.ps1 -StrictChangeAudit
.\tools\Test-ProjectFiles.ps1
```
