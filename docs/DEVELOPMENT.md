# 빌드와 개발 메모

## 요구 환경

- Windows와 Win32 콘솔
- Visual Studio/MSBuild 18 계열
- MSVC platform toolset `v145`
- Windows 10 SDK
- x64, C++20
- XAudio2를 포함한 Windows SDK 라이브러리

솔루션 파일은 `ConsoleGameProject.slnx`이며 Debug/Release, x64 구성만 정의되어 있다.

## 빌드 순서

현재 프로젝트는 import library와 공개 헤더를 중간 staging 디렉터리에 복사해 다음 프로젝트가 소비한다. 깨끗한 checkout에서는 다음 순서가 안전하다.

1. `SoundSystem`
2. `CraftEngine`
3. `ShootingGame`, `SokobanGame`, `Z1` 중 작업 대상 콘텐츠 프로젝트

Visual Studio에서 위 프로젝트를 차례로 빌드하면 된다. 이후 산출물이 존재하는 상태에서는 솔루션 전체 빌드도 가능하다. 현재 `ConsoleGameProject.slnx`에는 게임에서 CraftEngine으로 가는 의존성만 있고 CraftEngine에서 SoundSystem으로 가는 빌드 의존성은 명시되어 있지 않으므로, 완전한 클린 병렬 빌드는 순서 경쟁이 날 수 있다.

기본 출력 위치는 다음과 같다.

```text
Binaries/x64/<Debug|Release>/<Project>/
Intermediate/x64/<Debug|Release>/<Project>/
Libraries/<CraftEngine|SoundSystem>/<Debug|Release>/
```

## 실행과 상대 경로

엔진과 게임은 아래 상대 경로를 사용한다.

- 설정: `../Config/Setting.txt`
- 스테이지: `../Content/Stages/<파일>`
- 사운드: `../Content/Sound/<파일>`

Visual Studio의 프로젝트 디렉터리에서 실행하면 저장소 루트의 원본 Config/Content를 찾는다. 빌드된 EXE 디렉터리에서 실행하면 빌드 이벤트가 `Binaries/x64/<구성>/Config`와 `Content`에 복사한 파일을 찾는다. 다른 작업 디렉터리에서 직접 실행하면 설정 파일 open assertion이 발생하거나 Content 로드에 실패할 수 있다.

`Config/Setting.txt`는 목표 FPS, 화면 너비와 높이를 정의한다. 값은 양수여야 하며 알 수 없는 키는 현재 무시된다.

## 변경 위치 찾기

- 루프 순서와 시스템 조정: `CraftEngine/Engine/Engine.*`
- Actor 보유 및 지연 생성/삭제: `CraftEngine/Level/Level.*`
- Actor 이벤트와 Component 전달: `CraftEngine/Actor/Actor.*`
- 부모/자식, 로컬/월드 좌표: `CraftEngine/Component/TransformComponent.*`
- 화면 합성: `CraftEngine/Render/Renderer.*`, `Sprite.*`
- 충돌 판정: `CraftEngine/Physics/CollisionSystem.*`, `BoxComponent.*`
- 타일 저장·좌표 변환·점유 판정·Sprite 합성: `CraftEngine/Tilemaps/Tilemap.*`
- 타입 시스템: `CraftEngine/Core/CObject.h`, `CClass.h`
- 사운드 로드와 voice 수명: `SoundSystem/SoundSystem/Sound.*`
- Z1 맵 원본 파싱과 Tile 매핑: `Z1/World/OverworldMap.*`, `DungeonMap.*`, `CaveMap.*`
- 게임 규칙: 각 콘텐츠의 `Level/`, 구체 행동: 각 콘텐츠의 `Actor/`

## 기능 추가 패턴

### 새 Actor

1. 콘텐츠의 `Actor/`에 `Craft::Actor` 파생 타입을 둔다.
2. 기본 생성이 필요한 RTTI 타입이면 `TYPE_DECLARATIONS`를 선언한다.
3. 생성자에서 SpriteRenderer/Box 등 데이터 Component를 추가한다.
4. 다른 Actor나 Level이 필요한 작업은 `BeginPlay`에서 한다.
5. Level에서 `SpawnActor<T>`로 생성한다.
6. 새 파일을 프로젝트와 filters 파일에 등록한다.

### 새 Component

여러 Actor가 공유할 독립 데이터/동작 책임이 실제로 있을 때만 `ActorComponent`를 파생한다. 이벤트를 오버라이드하고 owner는 `GetOwner()`의 약한 참조를 잠가 사용한다. Transform은 Actor가 자동 생성하므로 일반 Component처럼 추가하지 않는다.

### Scene Graph 사용

자식 Actor를 Level에서 먼저 생성한 뒤 부모 Actor에 attach한다. 자식 생성자에 넘긴 위치를 부모 기준 오프셋으로 쓰려면 `AttachTo(parent, false)`, 현재 화면 위치를 유지하려면 기본값 `true`를 쓴다. 자식을 독립시키면서 화면 위치를 유지하려면 `DetachFromParent()`를 쓴다.

## 검증 체크리스트

- 엔진 또는 공개 헤더 변경: SoundSystem, CraftEngine, 두 게임을 순서대로 x64 빌드
- Transform 변경: 부모 이동 시 총구/엔진 이펙트가 함께 이동하는지, 발사 위치가 월드 좌표인지 확인
- 생명주기 변경: 프레임 중 spawn/destroy에서 순회 무효화나 파괴 Actor의 추가 이벤트가 없는지 확인
- 렌더링 변경: Sprite 투명 셀, X/Y 화면 경계 clipping, sorting order, buffer swap 확인
- 충돌 변경: 2D Box 크기/offset, 빠른 탄환, 파괴된 Actor, Component 없는 Actor 확인
- Tilemap 변경: 직사각형 맵 인덱싱, 셀/월드 좌표 변환, Map 경계와 blocked 점유 판정, 부분 구간 Sprite 합성 확인
- Z1 실행: 타이틀의 `Press Enter To Play` 안내 뒤 `Enter` 키로 새 게임을 시작하고, 방향키로 이동하며 HUD에 HP·검 보유 상태가 표시되는지 확인
- Z1 Overworld 변경: `Content/Z1/Maps/Overworld`를 출력 Content 경로에서도 읽는지, Room `(7, 7)` 배경과 플레이어가 보이는지 확인
- Z1 Overworld 변경: BlockingMap의 막힌 타일·Map 바깥으로 이동할 수 없고, 이동 가능한 타일에서는 플레이어 Box 크기만큼 정상 이동하는지 확인
- Z1 Overworld 변경: Room 전환 뒤 Player Box/Sprite가 새 Room 안에 완전히 보이는지, `(7,7) → (7,6) → (8,6) → (8,5) → (8,4) → (8,3) → (7,3)` 이외의 Room 경계는 넘을 수 없는지, 넉백으로도 제한 밖에 나가지 않는지 확인
- Z1 Room/Enemy 변경: 시작 Room에는 적이 없고, 다른 Room은 같은 좌표·시드에서 같은 계획으로 스폰되며 Room 전환 시 이전 적과 투사체가 제거되는지 확인
- Z1 동굴/던전 변경: 오버월드 `(7, 7)`의 검 동굴에서 Player Box가 검 타일에 닿을 때 검을 얻고 다시 나올 수 있는지, 검 없이 `(7, 3)`의 Dungeon 1 입구에 들어갈 수 없고 검 획득 후에는 던전 입구와 던전 출구가 정상 전환되는지 확인
- Z1 던전 변경: Room 전환 뒤 Player Box/Sprite가 새 Room 안에 완전히 보이고, 넉백 전환에도 같은 위치 보정이 적용되는지 확인
- Z1 전투 변경: `A` 검 공격이 이동 시 취소되고 공격당 한 번만 피해를 주는지, 풀 HP 검기와 Octorok 돌·Moblin 창·Aquamentus 화염구가 발사자 Box 가장자리에서 생성되는지 확인
- Z1 전투 변경: Player·Octorok·Moblin·Tektite의 8×5 Sprite와 Box가 함께 움직이는지, Tektite가 대기와 대각선 도약을 반복하며 BlockingMap 지형은 넘고 Player에 닿아 피해를 주는지, 피격된 Player의 넉백이 겹친 Tektite에 막히지 않는지 확인
- Z1 전투 변경: 투사체가 벽·Room 경계를 통과하지 않는지, 적 투사체가 맞는 방향의 Player 방패에 막히는지, 적 사망 이펙트가 보이는지 확인
- Z1 피해/클리어 변경: 벽을 통과하지 않는 넉백, 무적 시간 중 중복 피해 방지와 깜빡임, HP 0의 Game Over 전환, 보스 처치 후 Player Box가 하트·트라이포스 Sprite 영역에 닿을 때의 하트 회복과 Clear 전환을 확인
- Z1 사운드 변경: 검 획득, 검·검기 공격, 방패 방어, Player/일반 적/보스의 피격·사망 효과음과 Title·Overworld·Dungeon·Game Over·Clear BGM 전환을 확인
- Z1 사운드 변경: 트라이포스 획득 후 Zelda Is Rescued 팬파레가 끝난 뒤 Clear Level의 Ending Theme으로 전환되는지 확인
- 런타임 데이터 변경: 프로젝트 디렉터리와 출력 디렉터리 양쪽 실행 경로 확인

자동 테스트 프로젝트는 아직 없다. 문서만 바꾼 경우에는 Markdown 링크와 코드 식별자가 현재 트리와 맞는지 확인하고, 코드나 프로젝트 설정을 바꾼 경우에는 관련 실행 파일을 직접 구동해 확인한다.
