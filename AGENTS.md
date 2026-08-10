# CraftEngine 저장소 작업 지침

## 저장소 성격

이 저장소는 인프런의 [C++로 만드는 게임 엔진 프레임워크](https://www.inflearn.com/course/c-game-engine-framew?cid=341168)를 따라 만든 Windows 콘솔 게임 프로젝트다. 강의 예제를 그대로 보존하는 것보다 현재 저장소의 코드와 설계를 우선한다.

구조와 실행 흐름은 `docs/ARCHITECTURE.md`, 빌드 및 변경 방법은 `docs/DEVELOPMENT.md`를 먼저 참고한다.

## 프로젝트 경계

- `CraftEngine/`: 공용 엔진 DLL. 게임 루프, Level/Actor/Component, 입력, 콘솔 렌더링, 충돌, 커스텀 RTTI와 사운드 파사드를 담당한다.
- `SoundSystem/`: XAudio2 기반 독립 DLL. WAV 로드·캐시와 보이스 수명을 담당한다.
- `ShootingGame/`, `SokobanGame/`: 실행 가능한 콘텐츠 프로젝트. 게임 규칙과 구체 Actor/Level은 여기에 둔다.
- `Assets/`, `Config/`: 런타임 입력 데이터의 원본이다.
- `Includes/`, `Libraries/`, `Binaries/`, `Intermediate/`: 빌드가 생성하거나 복사하는 산출물이다. 직접 수정하거나 소스처럼 참조하지 않는다.

공용화할 현재 필요가 없는 게임 코드는 콘텐츠 프로젝트에 둔다. 두 콘텐츠가 실제로 공유해야 하거나 엔진 책임인 기능만 `CraftEngine`으로 올린다. 새 외부 의존성이나 새 추상화보다 표준 라이브러리와 기존 타입을 우선한다.

## 반드시 지킬 설계 규칙

### Transform과 Scene Graph

- 위치와 부모/자식 계층의 원본은 `TransformComponent`다. `Actor`에 별도의 위치나 부모/자식 상태를 추가하지 않는다.
- 모든 Actor는 생성 시 Transform 하나를 자동으로 가진다. `AddComponent<TransformComponent>`는 금지되어 있다.
- `Actor::GetPosition`/`SetPosition`은 로컬 좌표, `GetWorldPosition`은 계층을 반영한 월드 좌표의 편의 API다.
- 렌더링, 충돌, 월드 공간에서의 생성 위치는 월드 좌표를 사용한다. 부모 기준 배치는 로컬 좌표를 사용한다.
- 계층 변경은 `Actor::AttachTo`/`DetachFromParent` 또는 Transform의 동등 API를 사용한다. 직접 컨테이너를 조작하지 않는다.
- `AttachTo(parent, true)`는 기존 월드 위치를, `AttachTo(parent, false)`는 기존 로컬 오프셋을 보존한다. 순환 계층은 허용하지 않는다.
- Transform의 부모와 자식 링크는 모두 `weak_ptr`이다. 객체 수명은 Level과 필요한 콘텐츠 소유자가 관리한다.
- 자식 Actor를 함께 파괴하는 정책은 `Actor::Destroy`의 책임으로 남아 있다. 계층 계산 책임과 게임 객체 수명 정책을 혼합하지 않는다.

### 생명주기와 컬렉션 변경

- Actor는 반드시 `Level::SpawnActor`로 생성해 Level 소유권과 지연 추가 규칙을 적용한다.
- 프레임 순회 중 컨테이너를 직접 변경하지 않는다. 새 Actor와 Component는 요청 큐에 들어가 프레임 끝의 `ProcessRequestedActors`에서 반영된다.
- 생성자에서는 `AddComponent`로 구성만 하고, Level이나 다른 Actor가 필요한 초기화는 `BeginPlay`에서 한다.
- 오버라이드한 `BeginPlay`, `Tick`, `Draw`, `OnCollision`은 의도적인 완전 대체가 아니라면 `Super` 구현을 호출해 Component 이벤트 전달을 보존한다.
- `Destroy`는 즉시 Actor를 비활성화하고 실제 Level 목록 제거는 프레임 끝에 수행한다. 비활성 Actor를 다시 처리하지 않는다.
- 공용 타입 판별과 생성에는 기존 `TYPE_DECLARATIONS`, `IsA`, `Cast`, `TSubclassOf` 체계를 사용한다. 별도의 RTTI 체계를 만들지 않는다.

## 플랫폼과 빌드

- 대상은 Windows x64, C++20, MSVC toolset `v145`다. 렌더러는 Win32 콘솔 API, 사운드는 XAudio2를 사용한다.
- 의존 방향은 `SoundSystem -> CraftEngine -> ShootingGame/SokobanGame`이다. 현재 솔루션에는 CraftEngine의 SoundSystem 빌드 의존성이 명시되어 있지 않으므로 깨끗한 환경에서는 이 순서로 빌드한다.
- 헤더·라이브러리·DLL·Assets·Config 복사는 `.vcxproj`의 빌드 이벤트가 담당한다. 경로를 바꾸면 네 프로젝트의 include/library/output 및 복사 경로를 함께 확인한다.
- 새 `.cpp`/`.h` 파일을 추가하거나 제거할 때 해당 `.vcxproj`와 `.vcxproj.filters`도 함께 갱신한다.
- DLL 공개 API에는 기존 `CRAFT_API` 또는 `SOUND_SYSTEM_API` 경계를 유지한다.

## 변경 및 검증 원칙

- 실제 호출 흐름을 먼저 추적하고, 가장 작은 근본 수정으로 해결한다.
- 생성 산출물과 사용자 변경을 덮어쓰지 않는다. 관련 없는 정리나 대규모 포맷 변경을 섞지 않는다.
- 엔진 변경은 가능하면 두 게임을 모두 `Debug|x64`로 빌드해 확인한다. 콘텐츠 한정 변경은 해당 게임을 빌드하고, 공용 헤더나 DLL 경계를 건드렸다면 전체를 확인한다.
- 자동 테스트 프레임워크는 없다. 새 비자명 로직에는 기존 구조 안에서 실행 가능한 가장 작은 검증을 추가하고, 게임 동작 변경은 콘솔에서 직접 확인한다.
- 코드와 실제 구조가 달라지면 `docs/ARCHITECTURE.md` 또는 `docs/DEVELOPMENT.md`도 같은 변경에서 갱신한다.
