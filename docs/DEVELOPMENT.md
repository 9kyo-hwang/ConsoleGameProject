# 빌드와 개발

## 문서 범위

이 문서는 전체 솔루션에 적용되는 개발 환경, 빌드 경계, 변경 위치와 공통 검증 방법을 설명한다. 프로젝트별 게임 동선과 세부 회귀 시나리오는 해당 프로젝트 문서에서 관리한다. Z1 전용 검증은 [Z1 검증](z1/TESTING.md)을 참고한다.

## 요구 환경

- Windows와 Win32 콘솔
- Visual Studio/MSBuild 18 계열
- MSVC platform toolset `v145`
- Windows 10 SDK
- x64, C++20
- XAudio2와 WinSock2를 포함한 Windows SDK 라이브러리

솔루션 파일은 `ConsoleGameProject.slnx`이며 Debug/Release, x64 구성만 정의한다.

## 빌드 순서와 의존성

빌드가 공개 헤더와 라이브러리를 staging한 뒤 다음 프로젝트가 이를 참조한다. 깨끗한 checkout에서는 다음 순서가 안전하다.

1. `SoundSystem`
2. `CraftEngine`
3. `Sockets`
4. 작업 대상인 `ShootingGame`, `SokobanGame`, `Z1`, `Z1Server`

`Sockets`는 SoundSystem/CraftEngine과 독립적이므로 1~2보다 먼저 빌드해도 된다.

현재 `ConsoleGameProject.slnx`에는 콘텐츠 EXE에서 CraftEngine으로 가는 의존성과 Z1Server에서 Sockets로 가는 의존성이 있다. CraftEngine에서 SoundSystem, Z1에서 Sockets로 가는 의존성은 명시되어 있지 않다. 따라서 필요한 staging 산출물이 없는 상태에서 솔루션을 병렬 빌드하면 순서 경쟁이 날 수 있다.

기본 출력 위치는 다음과 같다.

```text
Binaries/x64/<Debug|Release>/<Project>/
Intermediate/x64/<Debug|Release>/<Project>/
Libraries/<CraftEngine|SoundSystem|Sockets>/<Debug|Release>/
Includes/<CraftEngine|SoundSystem|Sockets>/
```

## 실행과 상대 경로

런타임은 현재 작업 디렉터리를 기준으로 다음 상대 경로를 사용한다.

- 설정: `../Config/Setting.txt`
- Sokoban 스테이지: `../Content/Stages/<파일>`
- 공용 사운드: `../Content/Sound/<파일>`
- Z1 데이터: `../Content/Z1/<파일>`

`Z1Server`는 출력 디렉터리에서 `../Content/Z1/Maps/Overworld/BlockingMap.txt`를 읽는다. 따라서 해당 프로젝트의 pre-build event가 원본 `Content`를 `$(OutDir)..\\Content`로 복사한다. 이 복사 규칙을 바꾸면 Z1Server의 상대 경로와 함께 갱신한다.

Visual Studio의 프로젝트 디렉터리나 각 빌드 출력 디렉터리에서 실행하는 구성을 전제로 한다. 다른 작업 디렉터리에서 직접 실행하면 설정 assertion 또는 Content 로드 실패가 발생할 수 있다.

## 변경 위치 찾기

| 변경 대상 | 주요 위치 |
| --- | --- |
| 루프 순서와 시스템 초기화 | `CraftEngine/Engine/Engine.*` |
| Actor 보유와 지연 생성·삭제 | `CraftEngine/Level/Level.*` |
| Actor 이벤트와 Component 전달 | `CraftEngine/Actor/Actor.*` |
| 부모·자식과 로컬·월드 좌표 | `CraftEngine/Component/TransformComponent.*` |
| 화면 합성 | `CraftEngine/Render/Renderer.*`, `Sprite.*` |
| 충돌 판정 | `CraftEngine/Physics/CollisionSystem.*`, `BoxComponent.*` |
| 타일 저장·좌표·점유·합성 | `CraftEngine/Tilemaps/Tilemap.*` |
| 커스텀 RTTI | `CraftEngine/Core/CObject.h`, `CClass.h` |
| WAV와 voice 수명 | `SoundSystem/SoundSystem/Sound.*` |
| 소켓 자원과 주소 | `Sockets/Sockets/*` |
| Z1 wire 계약 | `Z1Shared/*` |
| Z1 클라이언트 transport와 Snapshot 표현 (`MyPlayer`, `NetworkPlayer`, `NetworkEnemy`) | `Z1/Network/*` |
| Z1 IOCP와 서버 simulation | `Z1Server/*` |
| 게임 규칙과 구체 Actor | 각 콘텐츠의 `Level/`, `Actor/` |

## 기능 추가 패턴

### 새 Actor

1. 콘텐츠의 `Actor/`에 `Craft::Actor` 파생 타입을 둔다.
2. 런타임 타입 정보가 필요하면 `TYPE_DECLARATIONS`를 선언한다.
3. 생성자에서는 SpriteRenderer, Box 등 자기 구성만 추가한다.
4. Level이나 다른 Actor가 필요한 작업은 `BeginPlay`에서 한다.
5. Level의 `SpawnActor<T>`로 생성한다.
6. 새 파일을 `.vcxproj`와 `.vcxproj.filters`에 등록한다.

### 새 Component

여러 Actor가 공유하는 실제 데이터·동작 책임이 있을 때만 `ActorComponent`를 파생한다. 이벤트를 오버라이드하고 owner는 `GetOwner()`의 약한 참조를 잠가 사용한다. Transform은 Actor가 자동 생성하므로 일반 Component처럼 추가하지 않는다.

### Scene Graph

자식 Actor를 Level에서 먼저 생성한 뒤 `AttachTo`를 사용한다. 부모 기준 오프셋을 유지하려면 `false`, 현재 월드 위치를 유지하려면 `true`를 전달한다. 부모·자식 컨테이너를 직접 변경하지 않는다.

### 네트워크 코드

범용 WinSock 자원 관리는 `Sockets`, Z1 packet 표현과 직렬화는 `Z1Shared`, IOCP와 권위형 상태는 `Z1Server`, 클라이언트 queue와 표현은 `Z1/Network`에 둔다. transport thread가 Actor나 Level을 직접 변경하지 않도록 한다.

## 공통 검증 원칙

| 변경 범위 | 최소 검증 |
| --- | --- |
| 문서 | Markdown UTF-8과 상대 링크 검사 |
| `.slnx`, `.vcxproj`, filters | 프로젝트 파일 등록 검사와 영향받는 프로젝트 빌드 |
| SoundSystem/CraftEngine 공개 경계 | DLL부터 모든 영향받는 콘텐츠까지 순서대로 빌드 |
| Transform·생명주기·렌더링·충돌·Tilemap | 관련 최소 장면과 세 콘텐츠의 핵심 회귀 확인 |
| Sockets/Z1Shared | Z1Server와 Z1 빌드, packet/framing 검증 |
| 콘텐츠 한정 변경 | 해당 EXE 빌드와 관련 수동 동작 확인 |
| 런타임 데이터 | 프로젝트 디렉터리와 출력 디렉터리 양쪽 경로 확인 |

자동화되지 않은 실행 확인을 빌드 성공으로 대체하지 않는다. 확인하지 않은 수동 동작은 완료로 기록하지 않는다.

## 저장소 검사 도구

Markdown 인코딩과 상대 링크는 다음 명령으로 검사한다. `-StrictChangeAudit`는 변경된 경로와 필수 문서의 동반 변경도 검사하며 `-Staged`는 staged 변경만 대상으로 한다.

```powershell
.\tools\Test-Documentation.ps1
.\tools\Test-Documentation.ps1 -StrictChangeAudit
.\tools\Test-Documentation.ps1 -Staged -StrictChangeAudit
```

소스 파일의 `.vcxproj`/`.vcxproj.filters` 등록과 솔루션 프로젝트 경로는 다음 명령으로 검사한다.

```powershell
.\tools\Test-ProjectFiles.ps1
```

Z1의 싱글플레이 동선과 서버 framing 검증은 [Z1 검증 문서](z1/TESTING.md)에 기록한다.
