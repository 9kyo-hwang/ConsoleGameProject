# CraftEngine 콘솔 게임 프로젝트

인프런의 [C++로 만드는 게임 엔진 프레임워크](https://www.inflearn.com/course/c-game-engine-framew?cid=341168)를 출발점으로 발전시킨 Windows 콘솔 게임 저장소다. 강의 예제 보존보다 현재 코드의 구조와 동작을 우선한다.

CraftEngine은 게임 루프와 Actor/Component 모델, 입력, 렌더링, 충돌, 타일맵과 사운드 파사드를 제공한다. 이 엔진 위에 ShootingGame, SokobanGame, Z1 콘텐츠가 있으며, Z1 멀티플레이 학습을 위한 공용 소켓 DLL과 IOCP 서버가 함께 있다.

## 솔루션 구성

| 경로 | 종류 | 역할 |
| --- | --- | --- |
| `SoundSystem/` | DLL | XAudio2 초기화, WAV 캐시와 보이스 수명 관리 |
| `CraftEngine/` | DLL | 게임 루프, Level/Actor/Component, 입력, 콘솔 렌더링, 충돌, 타일맵, 사운드 파사드 |
| `Sockets/` | DLL | WinSock 수명, 주소와 이동 전용 소켓 래퍼 |
| `ShootingGame/` | EXE | 실시간 비행 슈팅 콘텐츠 |
| `SokobanGame/` | EXE | 스테이지 파일 기반 소코반 콘텐츠 |
| `Z1/` | EXE | Zelda형 싱글플레이 콘텐츠와 네트워크 클라이언트 실험 |
| `Z1Server/` | EXE | Z1용 IOCP 서버와 권위형 시뮬레이션 |
| `Z1Shared/` | 헤더 | Z1 클라이언트와 서버가 공유하는 패킷·직렬화 계약 |

런타임 원본 데이터는 `Content/`와 `Config/`에 있다. `Includes/`, `Libraries/`, `Binaries/`, `Intermediate/`는 빌드가 생성하거나 복사하는 산출물이다.

## 빌드

대상 환경은 Windows x64, C++20, MSVC toolset `v145`다. 깨끗한 환경에서는 필요한 DLL을 먼저 staging해야 하므로 다음 순서로 빌드한다.

1. `SoundSystem`
2. `CraftEngine`
3. `Sockets`
4. 작업 대상 EXE

`Sockets`는 앞의 두 DLL과 독립적이므로 먼저 빌드해도 된다. 정확한 출력 경로와 현재 솔루션 의존성 제약은 [빌드와 개발](docs/DEVELOPMENT.md)을 참고한다.

## 문서

### 전체 솔루션

- [저장소 작업 규칙](AGENTS.md): 작업자가 반드시 지킬 설계·변경 규칙
- [솔루션 아키텍처](docs/ARCHITECTURE.md): 현재 프로젝트 경계, 의존 관계와 엔진 실행 구조
- [빌드와 개발](docs/DEVELOPMENT.md): 환경, 빌드 순서, 변경 위치와 공통 검증 방법

### 프로젝트별

- [Z1 문서 안내](docs/z1/README.md): Z1 싱글플레이·멀티플레이 구조, 맵 자료, 검증과 현재 진행 상태

프로젝트별 세부 동선이나 구현 계획은 해당 프로젝트 문서에서 관리한다. 전체 솔루션 문서에는 다른 프로젝트에도 적용되는 현재 계약만 기록한다.
