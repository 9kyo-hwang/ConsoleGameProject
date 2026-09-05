# 빌드와 개발

## 문서 범위

전체 솔루션에 적용되는 개발 환경, 빌드 경계, 변경 위치와 검증 방법을 기술합니다.

## 요구 환경

- Windows와 Win32 콘솔
- Visual Studio/MSBuild 18 계열
- MSVC platform toolset `v145`
- Windows 10 SDK
- x64, C++20
- XAudio2와 WinSock2를 포함한 Windows SDK 라이브러리

## 빌드 순서와 의존성

빌드를 통해 public 헤더 파일과 라이브러리들을 복사한 뒤 아래 프로젝트가 이를 참조합니다. 

아래 순서대로 빌드를 진행합니다.
1. `SoundSystem`
2. `CraftEngine`
3. `Sockets`
4. 작업 대상인 `ShootingGame`, `SokobanGame`, `Z1`, `Z1Server`

기본 출력 위치는 다음과 같습니다.

```text
Binaries/x64/<Debug|Release>/<Project>/
Intermediate/x64/<Debug|Release>/<Project>/
Libraries/<CraftEngine|SoundSystem|Sockets>/<Debug|Release>/
Includes/<CraftEngine|SoundSystem|Sockets>/
```

## 실행과 상대 경로

실행 시 현재 작업 디렉터리를 기준으로 아래와 같은 상대 경로를 사용합니다.

- 설정: `../Config/Setting.txt`
- Sokoban 스테이지: `../Content/Stages/<파일>`
- 사운드: `../Content/Sound/<파일>`
- Z1 데이터: `../Content/Z1/<파일>`

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
| 사운드 시스템 관련 | `SoundSystem/SoundSystem/Sound.*` |
| WinSock의 SOCKET, 주소 관련 | `Sockets/Sockets/*` |
| 게임 규칙과 구체 Actor | 각 콘텐츠의 `Level/`, `Actor/` |

## 기능 추가 패턴

### 새 Actor

1. 콘텐츠의 `Actor/`에 `Craft::Actor` 을 상속받은 타입을 추가합니다.
2. 런타임 타입 정보가 필요하면 `TYPE_DECLARATIONS`를 선언합니다.
3. 생성자에서는 SpriteRenderer, Box 등 본인에게 필요한 구성을 추가합니다.
4. Level이나 다른 Actor가 필요한 작업은 `BeginPlay`에서 수행합니다.
5. Level에 배치될 액터는 `SpawnActor<T>`로 생성합니다.
6. 새 파일을 `.vcxproj`와 `.vcxproj.filters`에 등록합니다.

### 새 Component

여러 Actor가 공유하는 데이터·동작 책임이 있을 경우 `ActorComponent`를 상속하고 필요한 이벤트 함수를 재정의합니다. Transform은 Actor가 자동 생성하므로 일반 Component처럼 추가하지 않습니다.

### Scene Graph

자식 Actor를 Level에서 먼저 생성한 뒤 `AttachTo`를 사용해 부모에 부착합니다. 부모 기준 오프셋을 유지하려면 `false`, 현재 월드 위치를 유지하려면 `true`를 전달합니다. 부모·자식 컨테이너를 직접 수정하지 않습니다.

### 네트워크 코드

범용 WinSock 자원 관리는 `Sockets`, Z1 packet 표현과 직렬화는 `Z1Shared`, IOCP와 권위형 상태는 `Z1Server`, 클라이언트 queue와 표현은 `Z1/Network`에 배치합니다. transport thread가 Actor나 Level을 직접 변경하지 않도록 주의합니다.

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

## 저장소 검사 도구

Markdown 문서 수정 뒤 아래 명령으로 검사할 수 있습니다. 인코딩, 문서 참조 링크를 검사합니다. `-StrictChangeAudit`는 변경된 참조 링크가 올바른지, 함께 수정돼야 하는 문서도 함께 변경되었는지 검사하며, `-Staged`는 깃 스테이징 상태의 파일들을 대상으로 검사합니다.

```powershell
.\tools\Test-Documentation.ps1
.\tools\Test-Documentation.ps1 -StrictChangeAudit
.\tools\Test-Documentation.ps1 -Staged -StrictChangeAudit
```

소스 파일의 `.vcxproj`/`.vcxproj.filters` 등록과 솔루션 프로젝트 경로는 다음 명령으로 검사할 수 있습니다.

```powershell
.\tools\Test-ProjectFiles.ps1
```
