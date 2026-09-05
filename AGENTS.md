# CraftEngine 저장소 작업 지침

## 문서 범위

이 파일은 저장소 전반에서 항상 지켜야 하는 작업 규칙만 정의한다. 현재 구조와 실행 흐름은 `docs/ARCHITECTURE.md`, 빌드와 공통 검증 방법은 `docs/DEVELOPMENT.md`를 따른다. Z1에만 해당하는 구조·동선·검증은 `docs/z1/`에서 관리한다.

## 프로젝트 경계

- `CraftEngine/`: 공용 엔진 DLL. 게임 루프, Level/Actor/Component, 입력, 콘솔 렌더링, 충돌, 타일맵, 커스텀 RTTI와 사운드 파사드를 담당한다.
- `SoundSystem/`: XAudio2 기반 독립 DLL. WAV 로드·캐시와 보이스 수명을 담당한다.
- `Sockets/`: WinSock 기반 독립 DLL. 네트워크 런타임, 주소와 소켓 자원 관리만 담당한다.
- `ShootingGame/`, `SokobanGame/`, `Z1/`: 실행 가능한 콘텐츠 프로젝트다. 게임 규칙과 구체 Actor/Level은 여기에 둔다.
- `Z1Server/`: Z1 전용 서버 실행 파일이다. IOCP Session과 권위형 월드 상태를 담당하며 CraftEngine에 의존하지 않는다.
- `Z1Shared/`: Z1과 Z1Server가 공유하는 wire format과 직렬화 헤더다. 렌더링이나 Actor 타입을 넣지 않는다.
- `Content/`, `Config/`: 런타임 입력 데이터의 원본이다.
- `Includes/`, `Libraries/`, `Binaries/`, `Intermediate/`: 빌드가 생성하거나 복사하는 산출물이다. 직접 수정하거나 소스처럼 참조하지 않는다.

공용화할 현재 필요가 없는 게임 코드는 콘텐츠 프로젝트에 둔다. 둘 이상의 소비자가 실제로 같은 계약을 사용하거나 기반 시스템의 책임일 때만 공용 계층으로 올린다. 새 외부 의존성이나 추상화보다 표준 라이브러리와 기존 타입을 우선한다.

## 반드시 지킬 설계 규칙

### Transform과 Scene Graph

- 위치와 부모/자식 계층의 원본은 `TransformComponent`다. `Actor`에 별도의 위치나 부모/자식 상태를 추가하지 않는다.
- 모든 Actor는 생성 시 Transform 하나를 자동으로 가진다. `AddComponent<TransformComponent>`는 금지되어 있다.
- `Actor::GetPosition`/`SetPosition`은 로컬 좌표, `GetWorldPosition`은 계층을 반영한 월드 좌표의 편의 API다.
- 렌더링, 충돌, 월드 공간에서의 생성 위치는 월드 좌표를 사용한다. 부모 기준 배치는 로컬 좌표를 사용한다.
- 계층 변경은 `Actor::AttachTo`/`DetachFromParent` 또는 Transform의 동등 API를 사용한다. 직접 컨테이너를 조작하지 않는다.
- `AttachTo(parent, true)`는 기존 월드 위치를, `AttachTo(parent, false)`는 기존 로컬 오프셋을 보존한다. 순환 계층은 허용하지 않는다.
- Transform의 부모와 자식 링크는 모두 `weak_ptr`이다. 객체 수명은 Level과 필요한 콘텐츠 소유자가 관리한다.
- 자식 Actor를 함께 파괴하는 정책은 `Actor::Destroy`의 책임으로 남겨 계층 계산과 객체 수명 정책을 섞지 않는다.

### 생명주기와 컬렉션 변경

- Actor는 반드시 `Level::SpawnActor`로 생성해 Level 소유권과 지연 추가 규칙을 적용한다.
- 프레임 순회 중 컨테이너를 직접 변경하지 않는다. 새 Actor와 Component는 요청 큐를 거쳐 프레임 끝에 반영한다.
- 생성자에서는 `AddComponent`로 구성만 하고, Level이나 다른 Actor가 필요한 초기화는 `BeginPlay`에서 한다.
- 오버라이드한 `BeginPlay`, `Tick`, `Draw`, `OnCollision`은 의도적인 완전 대체가 아니라면 `Super` 구현을 호출한다.
- `Destroy`는 즉시 Actor를 비활성화하고 실제 Level 목록 제거는 프레임 끝에 수행한다. 비활성 Actor를 다시 처리하지 않는다.
- 공용 타입 판별과 생성에는 `TYPE_DECLARATIONS`, `IsA`, `Cast`, `TSubclassOf`를 사용한다. 별도의 RTTI 체계를 만들지 않는다.

### 네트워크 경계

- `Sockets`에는 Z1 패킷, Session 정책, IOCP 서버 루프나 게임 규칙을 넣지 않는다.
- `Z1Server`는 CraftEngine의 Actor, Renderer, Input, Sound를 링크하지 않는다.
- network thread와 IOCP completion 처리는 Actor나 Level을 직접 변경하지 않는다. 클라이언트는 main-thread queue, 서버는 명시적인 simulation 경계를 거친다.
- 서버가 클라이언트가 보낸 위치·HP·소유 상태를 그대로 신뢰하지 않는다. wire 입력은 길이와 범위를 먼저 검증한다.

## 플랫폼과 빌드

- 대상은 Windows x64, C++20, MSVC toolset `v145`다. 렌더러는 Win32 콘솔 API, 사운드는 XAudio2, 네트워크는 WinSock/IOCP를 사용한다.
- `CraftEngine`은 `SoundSystem`을 사용한다. 콘텐츠 EXE는 `CraftEngine`을 사용하며, Z1과 Z1Server는 `Sockets`와 `Z1Shared`도 사용한다.
- 현재 솔루션에는 일부 빌드 의존성이 빠져 있으므로 깨끗한 환경에서는 `SoundSystem`, `CraftEngine`, `Sockets`, 대상 EXE 순으로 빌드한다.
- 헤더·라이브러리·DLL·Content·Config 복사는 `.vcxproj` 빌드 이벤트가 담당한다. 경로 변경 시 영향을 받는 모든 프로젝트의 include/library/output 및 복사 경로를 확인한다.
- 새 `.cpp`/`.h` 파일을 추가하거나 제거할 때 해당 `.vcxproj`와 `.vcxproj.filters`도 함께 갱신한다.
- DLL 공개 API에는 기존 `CRAFT_API`, `SOUND_SYSTEM_API`, `SOCKET_API` 경계를 유지한다.

## 변경 및 검증 원칙

- 실제 호출 흐름을 먼저 추적하고 가장 작은 근본 수정으로 해결한다.
- 생성 산출물과 사용자 변경을 덮어쓰지 않는다. 관련 없는 정리나 대규모 포맷 변경을 섞지 않는다.
- 공용 엔진 변경은 가능하면 세 콘텐츠를 모두 `Debug|x64`로 빌드한다. DLL 공개 경계나 솔루션 설정 변경은 영향을 받는 모든 소비자를 확인한다.
- 자동 테스트 프레임워크는 없다. 새 비자명 로직에는 현재 구조에서 실행 가능한 가장 작은 검증을 추가하고, 게임 동작은 해당 프로젝트의 수동 검증 문서를 따른다.
- 솔루션의 현재 구조가 달라지면 `docs/ARCHITECTURE.md` 또는 `docs/DEVELOPMENT.md`를 갱신한다. Z1 전용 계약이나 진행 상태가 달라지면 `docs/z1/`의 해당 문서를 갱신한다.
