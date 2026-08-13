# Zelda형 콘솔 게임 개발 계획

## 문서 목적

이 문서는 CraftEngine을 이용해 초대 `The Legend of Zelda`에서 영감을 받은 소규모 콘솔 게임을 만들기 위한 구현 브리프다. 다음 작업자가 저장소 구조를 다시 조사하지 않고도 엔진 변경 범위, 콘텐츠 책임, 작업 순서와 검증 기준을 파악하는 것을 목표로 한다.

원작 지상 맵 자료의 Room 크기와 타일 포맷을 채택한 근거 및 세부 결정은 [`ZELDA_MAP_DATA_REFERENCE.md`](ZELDA_MAP_DATA_REFERENCE.md)에 정리한다.

목표 콘텐츠는 다음으로 한정한다.

- 화면 가장자리로 이동해 왕복할 수 있는 고정 화면 필드 몇 개
- 체력과 무기 소지 상태를 가진 4방향 플레이어
- 플레이어의 근접 공격과 피격 후 짧은 무적 시간
- 서로 다른 행동을 가진 일반 적 몇 종
- 마지막 필드의 보스
- 타이틀, 게임플레이, 클리어 화면

원작의 전체 월드, 던전, 아이템 경제, 세이브, 연속 스크롤을 재현하는 프로젝트가 아니다. 원작 이미지와 사운드를 복사하지 않고 자체 제작 문자 아트와 허용된 에셋을 사용한다.

## 핵심 결론

첫 완성본을 위해 CraftEngine에 반드시 필요한 공용 변경은 두 가지다.

1. 한 줄 문자열 렌더링을 투명도와 셀별 색상을 가진 `N x M` 콘솔 스프라이트로 확장한다.
2. 가로 폭만 있는 충돌 박스를 크기와 오프셋을 가진 2차원 AABB로 확장한다.

필드 데이터, 지형 통행 판정, 체력, 무기, 공격, 적 AI, 보스와 화면 상태는 새 콘텐츠 프로젝트에 둔다. 화면보다 큰 Overworld의 월드 좌표를 고정 Room 화면에 표시하기 위한 Renderer View 오프셋만 공용으로 사용하며, 추적·보간·줌을 가진 범용 카메라, 물리 반응, 범용 타일맵 엔진, ECS와 범용 애니메이션 시스템은 현재 범위에 넣지 않는다.

## 현재 엔진에서 확인한 계약

### 프레임과 생명주기

`CraftEngine/Engine/Engine.cpp`의 한 갱신 프레임은 입력, Level 초기화, BeginPlay, Tick, 충돌, Draw, Level 교체, 요청 반영, 이전 상태 저장 순서로 동작한다.

- Actor는 반드시 `Level::SpawnActor`로 생성한다.
- 프레임 중 생성한 Actor는 즉시 반환되지만 같은 프레임의 Tick, 충돌, Draw에는 참여하지 않는다.
- `Destroy`는 즉시 비활성화하고 실제 Level 목록 제거는 프레임 끝에 수행한다.
- 방 교체 중 기존 Actor를 파괴하고 새 Actor를 생성하면 한 프레임 동안 새 Actor가 보이지 않을 수 있다. 짧은 암전이나 전환 상태로 이를 감춘다.
- 파생 `BeginPlay`, `Tick`, `Draw`, `OnCollision`은 의도적인 완전 대체가 아니라면 `Super`를 호출한다.

### Transform과 Scene Graph

- 모든 Actor는 생성자에서 Transform 하나를 자동으로 가진다.
- `GetPosition`과 `SetPosition`은 로컬 좌표다.
- 렌더링, 충돌, 독립 Actor 생성 위치는 월드 좌표를 사용한다.
- Transform은 정수 이동만 지원하며 이번 범위에는 충분하다.
- `AttachTo(parent, false)`는 로컬 위치를 부모 기준 오프셋으로 보존한다.
- Transform 계층은 수명을 소유하지 않으며 부모 Destroy는 자식 파괴를 전파한다.

검 공격처럼 플레이어를 따라야 하는 객체는 Level에서 Spawn한 뒤 플레이어에 attach한다. 플레이어에서 분리되어 계속 이동하는 투사체는 월드 위치에서 독립 Actor로 Spawn한다.

### 렌더링

`Renderer::RenderCommand`는 문자열 payload와 `Sprite` payload를 함께 지원한다. `Sprite`는 크기와 `SpriteCell` 배열을 가지며, 각 셀은 glyph, Win32 attribute와 투명 여부를 가진다. `DrawRenderQueue`는 공통 셀 순회·클리핑 경로로 X/Y 양축을 처리하고, 투명 셀은 기존 백버퍼 셀을 보존한다. Sprite는 저장된 콘솔 셀 크기 그대로 1:1로 출력한다.

`Submit`은 HUD와 메뉴의 화면 좌표를 사용하고, `SubmitWorld`는 현재 View의 `worldOrigin`과 `screenOrigin`으로 월드 좌표를 화면 좌표로 변환한다. `SpriteRendererComponent`는 문자열과 Sprite 모두 Actor 월드 위치로 제출한다.

프레임은 `CHAR_INFO[]`와 셀별 sorting order 배열로 구성되므로 백엔드를 교체하지 않고 2차원 셀 합성을 수행한다. 높은 sorting order가 이기며 값이 같으면 나중 명령이 덮어쓴다. `ScreenBuffer`의 Right/Bottom은 inclusive 좌표 규칙에 맞게 `size - 1`을 사용한다.

구현 상태: 완료. Z1에서 다중 셀 Sprite, 투명 셀, 음수 위치와 화면 경계 클리핑을 직접 확인했고, 기존 ShootingGame/SokobanGame 출력과 플레이를 회귀 확인했다.

### 충돌

`BoxComponent`는 `Vector2 size`와 `Vector2 offset`을 저장한다. 기존 `BoxComponent(int width)`, `GetWidth`와 `SetWidth`는 `size.x` 기반 호환 API로 유지되며, width 생성자는 `(width, 1)` 크기와 `(0, 0)` offset으로 동작한다. `CollisionSystem`은 Box가 있는 활성 Actor의 모든 쌍에 대해 이전/현재 월드 위치를 감싸는 내부 `SweptBounds`를 계산하고 X/Y 포함 범위를 비교한다. 유효하지 않은 크기(한 변이 0 이하)는 충돌하지 않는 것으로 처리한다.

충돌은 겹침 이벤트만 보내고 물리 반응은 제공하지 않는다. 이 성질을 유지해 책임을 다음처럼 나눈다.

- 정적 지형 통행: Room 타일 데이터에 질의
- 플레이어, 적, 검, 투사체의 피해: BoxComponent와 OnCollision

바닥과 장식 타일을 각각 Actor로 만들지 않는다. 하나의 방 배경 Sprite와 별도 통행 데이터로 표현해 Actor 수와 충돌 쌍을 작게 유지한다.

### 입력, 사운드와 Level

- Input의 가상 키 Down, Up, Hold API는 첫 버전에 충분하다.
- 원샷과 단일 반복 BGM 파사드도 첫 버전에 충분하다.
- `Engine::AddNewLevel<T>`는 현재 프레임 Draw 후 다음 Level로 교체한다.
- 새 콘텐츠는 SokobanGame의 직접 `mainLevel` 교체보다 예약 전환을 우선한다.

## 기본 화면과 좌표 정책

- 추적·보간 없는 고정 Room View 방식을 사용한다.
- 상단 일부 행은 HUD, 나머지는 하나의 Room이 들어가는 플레이 영역이다.
- 원작 자료의 `16 x 16`은 NES 픽셀 단위이며 CraftEngine 콘솔 셀 크기가 아니다.
- Room의 논리 격자는 기본 `16 x 11` 타일로 고정한다.
- 현재 Overworld 구현에서 논리 타일 하나의 콘솔 footprint는 `10 x 5` 셀이다. 이는 콘솔 글자 비율 보정값이며, Renderer의 모든 Sprite 크기를 제한하는 규칙이 아니다.
- Transform과 collider는 전체 Map 기준 월드 셀 단위를 사용하고 Sprite는 실제 콘솔 셀 크기를 사용한다.
- 부드러운 속도가 필요하면 ShootingGame처럼 Actor 내부에 float 위치를 누적하고 Transform에는 정수 결과만 기록한다.

현재 View는 Room의 월드 좌상단을 화면 플레이 영역의 좌상단으로 옮기는 오프셋만 제공한다. 연속 스크롤, 추적, 보간, 줌이나 별도 viewport clipping이 필요해질 때 범용 Camera를 검토한다.

### 논리 타일과 렌더 셀

- Room txt의 토큰 하나는 콘솔 셀 하나나 Actor 하나가 아니라 논리 타일 ID다. 기본 파일은 `16`개 토큰씩 `11`행을 갖고, 토큰은 공백으로 구분된 16진수 ID다.
- 타일 ID의 시각 정보는 `OverworldLevel`이 소유한 TileId-Sprite map에서 조회하고, 통행 정보는 전체 `OverworldMap`의 Cell Grid에서 조회한다.
- 논리 타일은 Z1의 `MapTileSize`에 따라 콘솔 셀로 확장한다. 현재 구현은 `10 x 5`이며, 원작의 `16 x 16` 픽셀을 직접 의미하지 않는다.
- 정적 지형은 타일 Sprite를 하나의 방 배경 Sprite로 합성하고, 통행 판정은 별도 BlockingMap에서 수행한다.
- 플레이어, 적, 아이템과 보스처럼 동작이 필요한 대상만 Spawn Actor로 만든다.
- 출구, 진입 위치와 스폰은 지형 타일 파일과 분리된 Room 메타데이터로 관리한다. 지형과 엔티티 토큰을 한 파일에 섞지 않는다.

## 엔진 변경 사양

### 1. 2D 셀 Sprite

정확한 타입명은 구현 시 조정할 수 있지만 Sprite는 크기와 셀 배열을, 각 셀은 문자, 전경/배경 속성과 투명 여부를 가져야 한다.

필수 계약:

- 셀 개수는 `size.x * size.y`와 일치한다.
- 투명 셀은 프레임 문자, 색상과 sorting order를 바꾸지 않는다.
- Renderer는 X와 Y 양쪽의 부분 클리핑을 지원한다.
- 기존 sorting order 계약을 유지한다.
- 기존 `Submit(string, ...)`과 문자열 기반 SpriteRendererComponent 생성자를 호환 API로 유지한다.
- SpriteRendererComponent에서 Sprite 교체와 전체 크기 조회가 가능해야 한다.
- Windows 콘솔 백엔드와 이중 버퍼 구조를 유지한다.

배경색 표현은 구현 전에 `Color`를 확장할지 셀에 Win32 attribute를 저장할지 결정한다. 어느 쪽이든 기존 게임 호출부가 깨지지 않아야 한다. 같은 작업에서 `ScreenBuffer::Draw`의 `SMALL_RECT` Right/Bottom이 inclusive 좌표라는 점도 검증한다.

검증 항목:

- 3행 이상 Sprite와 셀별 색상
- 투명 셀 뒤에 낮은 sorting order의 배경이 보임
- 상하좌우 부분 클리핑과 완전한 화면 밖 명령 무시
- 같은 sorting order의 기존 제출 순서
- ShootingGame과 SokobanGame의 한 줄 이미지 회귀 없음

구현 상태: 완료. 기존 문자열 제출 경로를 유지하면서 Sprite payload, 투명 셀 합성, 셀별 attribute, 양축 클리핑을 구현했다.

### 2. 2D BoxComponent

`BoxComponent(int width)`와 `GetWidth`는 호환을 위해 유지하고 내부 표현은 `Vector2 size`와 `Vector2 offset`으로 확장한다.

필수 계약:

- width 생성자는 `size = { width, 1 }`, `offset = { 0, 0 }`으로 동작한다.
- 크기 생성자와 size/offset getter 및 setter를 제공한다.
- 충돌 기준점은 `Actor::GetWorldPosition() + offset`이다.
- 유효한 충돌 크기는 양수여야 한다. 기존 기본/width 생성자의 0은 호환을 위해 허용하되 충돌하지 않는 영역으로 처리한다.
- swept 범위는 X와 Y에서 각각 이전/현재 위치를 포함한다.
- 충돌 쌍을 먼저 모은 후 콜백을 보내는 현재 순서를 유지한다.

검증 항목:

- 높이 2 이상인 Actor의 겹침과 비겹침
- Y축 고속 이동
- 양수/음수 collider offset
- Destroy된 Actor의 추가 콜백 방지
- 기존 높이 1 Actor의 회귀 없음

구현 상태: 엔진 구현 완료. Z1의 2D 충돌 케이스 수동 확인이 남아 있다.

### 3. Level 전환 API

Title, Gameplay, Clear는 현재 API로 구현 가능하므로 Level 개편은 선행 조건이 아니다. Level 생성자에 세션 전달이 필요해지면 `AddNewLevel<T, Args...>` 형태의 인자 전달만 우선 추가한다. Level 스택, 범용 GameInstance와 Sokoban 메뉴 전환 통합은 별도 요구가 생길 때 진행한다.

## 콘텐츠 구조

새 프로젝트는 아래 책임 구분을 따른다. 실제 프로젝트 이름은 `Z1`으로 확정했다.

```text
Z1/
├─ Actor/       Pawn, Player, SwordAttack, Enemy, 적 파생 타입, 투사체, Boss
├─ Game/        Z1Game, GameSession
├─ Level/       TitleLevel, OverworldLevel, ClearLevel
├─ World/       OverworldMap
├─ UI/          Hud
└─ Main.cpp
```

### GameSession 또는 RunState

화면 전환에도 유지할 최소 진행 상태가 필요할 때만 사용한다. Room 이동 자체를 위해 필수인 객체는 아니다.

- 현재/최대 체력
- 무기 보유 여부
- 보스 처치와 클리어 여부

현재 Area와 Room, Room 전용 Actor 목록은 `OverworldLevel`과 `AreaManager`가 소유한다. Level, Actor 또는 Renderer 포인터는 GameSession에 저장하지 않는다. 저장 기능이 없고 GameplayLevel을 유지하는 첫 버전에서는 GameSession을 생략하고 파생 Game이 작은 `RunState`만 소유해도 된다.

### Level과 Room

- TitleLevel: 시작 입력과 타이틀
- OverworldLevel: 플레이어, 현재 Room, 전투와 방 전환
- ClearLevel: 클리어 메시지와 종료 또는 재시작
- Area: 여러 Room을 묶는 지상, 동굴 또는 던전 단위의 콘텐츠 그룹
- Room: 별도 데이터 객체가 아니라 전체 Map에서 현재 화면에 표시하는 16×11 논리 타일 구간

필드마다 Level을 만들지 않는다. Area는 하나 이상의 Room을 포함하며, 동굴이나 던전도 별도 Level이 아니라 Area 데이터로 표현할 수 있다. Overworld는 `Content/Z1/Maps/Overworld`의 `256 x 88` 원본 TileId/Blocking 맵을 `OverworldMap` 하나가 읽어 2차원 Cell Grid로 보관한다. 현재 Room의 `16 x 11` TileId는 별도 데이터로 추출하지 않고 전체 Map 좌표로 직접 조회한다. `OverworldLevel`의 TileId-Sprite map에서 찾은 문자 Sprite를 `MapTileSize (10, 5)`만큼 펼쳐 실제 Room 배경을 만들고, 통행 판정은 전체 Map에 Pawn의 월드 Box를 질의한다. Player의 전역 월드 좌표가 다른 Room 영역에 들어가면 배경과 Renderer View를 교체하며 Player Transform은 유지한다. 상세한 외부 자료 대응과 포맷은 [`ZELDA_MAP_DATA_REFERENCE.md`](ZELDA_MAP_DATA_REFERENCE.md)를 따른다.

### 플레이어와 공격

`Pawn`은 Player와 Enemy가 공유하는 체력, facing, 이동 속도 누산, 피해·사망, 넉백과 피격 무적 상태를 가진다. Player는 현재 프레임의 이동 입력과 마지막 facing을 분리해 유지하며, 무기 보유와 활성 공격을 관리한다.

검은 짧은 수명의 별도 SwordAttack Actor로 구현한다.

1. 방향에 맞는 로컬 오프셋으로 Spawn한다.
2. `AttachTo(player, false)`로 연결한다.
3. 방향별 Sprite와 2D BoxComponent를 사용한다.
4. 피해 가능한 대상과 충돌하면 콘텐츠 피해 API를 호출한다.
5. 공격 시간이 끝나면 Destroy한다.

같은 공격이 한 적에게 매 프레임 피해를 주지 않도록 공격별 적중 기록 또는 피해 무적 정책을 명시한다.

### 적과 보스

`Enemy`는 `Pawn`을 상속하고 공통 사망 처리와 적 행동의 확장 지점만 가진다. 이동과 공격 패턴은 구체 타입에 둔다. 현재 단일 Enemy는 Player를 향해 한 축씩 추적하며, 두 번째 적 타입부터 추적형·배회형·원거리형으로 분리한다. 행동 트리 대신 작은 상태 enum과 타이머를 사용한다.

보스도 이동, 투사체 또는 접촉 공격, 높은 체력과 사망 처리부터 시작한다. 보스가 죽으면 Session의 클리어 상태를 설정하고 ClearLevel을 예약한다.

## Room 전환 절차

현재 인접 Overworld Room은 별도 출구 메타데이터 없이 연속된 전체 Map 좌표로 이동한다.

1. Player의 후보 월드 Box를 전체 BlockingMap에 질의한다.
2. 이동 가능하면 후보 월드 좌표가 속한 `RoomCoordinate`를 계산한다.
3. Room이 달라졌으면 전체 Map에서 새 16×11 구간의 TileId를 직접 조회해 배경 Sprite를 다시 만든다.
4. Player의 월드 위치를 그대로 이동한다.
5. Draw에서 새 Room 월드 원점을 Renderer View에 설정한다.

현재 Enemy는 Room 변경 시 기존 인스턴스를 Destroy하고 `EnemySpawner`가 새 Room 좌표와 월드 시드로 만든 계획에 따라 다시 Spawn한다. 아이템, 입력 잠금과 전환 연출은 아직 없으며 동굴과 특수 워프에는 별도 메타데이터가 필요하다.

## 구현 순서와 완료 기준

### 단계 0: 프로젝트 골격

- 새 x64 C++20 Application 프로젝트와 솔루션 의존성 추가
- CraftEngine 헤더/library/DLL 및 Content 복사 설정 연결
- Title, 빈 Gameplay, Clear 전환

완료: 새 프로젝트가 Debug|x64로 빌드되고 세 화면을 키 입력으로 오간다.

### 단계 1: 2D 렌더링

- Sprite/셀 데이터, 2D 합성, 투명도, 양축 클리핑
- SpriteRendererComponent 확장과 문자열 API 호환

완료: 다색 Sprite가 올바르게 겹치고 기존 두 게임 출력이 유지된다.

현재 상태: 완료. Z1 Sprite 출력과 경계 클리핑, 기존 두 게임의 빌드 및 플레이 회귀를 확인했다.

### 단계 2: 2D 충돌

- BoxComponent 크기/오프셋
- CollisionSystem X/Y swept AABB

완료: 플레이어, 검과 적의 Y축 포함 충돌이 예상대로 동작한다.

현재 상태: BoxComponent와 CollisionSystem 구현 및 Z1 개발 장면의 1x1/2x2 겹침, X/Y 분리, offset, 이전/현재 위치 사이의 swept 이동 검증을 완료했다.

### 단계 3: 한 방 전투 버티컬 슬라이스

- 고정 `MapTileSize(10, 5)`, 논리 타일 좌표와 월드 셀 좌표 변환
- `256 x 88` Overworld TileId/Blocking 파일을 읽고 `16 x 11` Room 하나를 추출
- TileId/Sprite 연결, 4방향 플레이어, 검, 적 하나, 체력 HUD와 사망

완료: 이동, 공격, 피격, 적 사망과 플레이어 사망의 전체 루프가 동작한다.

현재 상태: Player 검 공격, Enemy 피해·사망, 공통 넉백과 피격 무적까지 구현했다. Enemy 공격, Player 사망 흐름과 체력 HUD는 남아 있다.

### 단계 4: Room 전환

- 전역 월드 좌표 기반 인접 Room 왕복과 View 전환
- Room Actor 재생성, 플레이어 상태 유지, 동굴·워프 메타데이터

완료: 반복 이동해도 Actor 중복, 플레이어 소실, 잘못된 충돌이나 상태 초기화가 없다.

### 단계 5: 적과 보스

- 일반 적 2~3종, 필요한 투사체, 마지막 Room 보스

완료: 동일한 피해 계약을 사용하고 보스 처치가 ClearLevel로 이어진다.

### 단계 6: 화면과 회귀 검증

- 타이틀, HUD, 게임 오버/재시작, 클리어와 사운드
- Content 및 Config 경로, 문서와 실제 구조 동기화

완료: 프로젝트 디렉터리와 출력 디렉터리 양쪽에서 실행되고 기존 콘텐츠 회귀가 없다.

## 빌드 및 수동 검증

엔진 공개 API 변경 후 `Debug|x64` 빌드 순서는 SoundSystem, CraftEngine, ShootingGame, SokobanGame, Z1이다. 새 소스는 해당 `.vcxproj`와 `.vcxproj.filters`에 등록하고 공개 헤더 복사도 확인한다.

수동 점검:

- 기존 게임의 클리핑, 색상, sorting order와 높이 1 충돌
- 2D Sprite 투명도와 네 방향 클리핑
- 2D collider 크기와 오프셋
- 한 공격의 중복 피해 정책
- Room 왕복 시 Actor 수명과 플레이어 상태
- 보스 사망 후 클리어 화면

## 보류 항목과 확장 조건

| 보류 기능 | 추가를 검토할 조건 |
| --- | --- |
| 추적 Camera/Viewport | 연속 스크롤, 보간, 줌 또는 Room 영역 clipping이 실제 요구가 됨 |
| float Transform | 정수 셀 이동이 게임플레이 결함을 만듦 |
| 충돌 레이어/마스크 | 불필요한 콜백이나 검사 비용이 관측됨 |
| 공간 분할 | 프로파일링에서 O(n²) 충돌이 병목임 |
| 범용 애니메이션 | 여러 콘텐츠가 같은 프레임 재생 계약을 공유함 |
| 입력 매핑 | 키 변경 또는 여러 입력 장치가 요구됨 |
| Level 스택/GameInstance | 여러 콘텐츠가 공통 상태 전달을 요구함 |
| 저장/불러오기 | 세션 간 진행 보존이 범위에 포함됨 |

## 작업자 체크리스트

- 공용 엔진 책임과 Z1 콘텐츠 책임을 구분했는가?
- 기존 문자열 렌더링과 width 기반 Box API를 보존했는가?
- 렌더링과 충돌에 월드 좌표를 사용했는가?
- Actor를 SpawnActor로 만들고 지연 추가를 고려했는가?
- Actor 컨테이너를 프레임 중 직접 수정하지 않는가?
- 필요한 Super 호출을 보존했는가?
- 논리 타일, 콘솔 렌더 셀과 동적 Actor를 구분했는가?
- `MapTileSize`를 Renderer에 하드코딩하지 않고 Z1의 배경 합성과 Map 통행 판정에서 공유하는가?
- Room 파일이 `16 x 11` 16진수 타일 포맷을 검증하고, 출구/스폰 메타데이터와 분리되어 있는가?
- Room 배경 타일을 개별 Actor로 만들지 않았는가?
- vcxproj, filters와 구조 문서를 함께 갱신했는가?
- 엔진 변경 뒤 기존 두 게임을 빌드하고 직접 확인했는가?

기본 결정과 다른 선택이 필요하면 코드만 우회하지 말고 변경 이유, 새 계약과 검증 방법을 이 문서 또는 `docs/ARCHITECTURE.md`에 함께 기록한다.
