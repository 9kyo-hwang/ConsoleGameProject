# Z1 문서 안내

## 범위

이 디렉터리는 `Z1`, `Z1Server`, `Z1Shared`와 Z1이 사용하는 네트워크 경계에만 적용되는 문서를 모은다. 전체 솔루션의 공통 엔진 구조와 빌드는 각각 [솔루션 아키텍처](../ARCHITECTURE.md), [빌드와 개발](../DEVELOPMENT.md)을 기준으로 한다.

## 현재 프로젝트

Z1은 Zelda형 고정 화면 콘텐츠를 CraftEngine 위에 구현한 프로젝트다. 타이틀에서 새 게임을 시작해 오버월드의 검 동굴을 거쳐 Dungeon 1의 보스를 처치하고 클리어하는 싱글플레이 흐름이 완성되어 있다.

별도로 localhost 기반 서버 권위형 멀티플레이를 단계적으로 구현하고 있다. 현재는 Z1 클라이언트 연결, 입력 전송, 서버의 Player 이동, Snapshot 수신과 원격 `NetworkPlayer` 표현까지 연결되어 있으며 싱글플레이 전투를 서버 simulation으로 옮기는 작업은 완료되지 않았다.

## 문서 역할

| 문서 | 역할 | 갱신 시점 |
| --- | --- | --- |
| [ARCHITECTURE.md](ARCHITECTURE.md) | 현재 싱글플레이 구조와 Z1 내부 책임 | Level, Actor, 맵 또는 상태 흐름 변경 |
| [TESTING.md](TESTING.md) | Z1 빌드·실행과 수동/스크립트 검증 | 관찰 가능한 동작이나 검증 방법 변경 |
| [MAP_DATA.md](MAP_DATA.md) | 외부 맵 자료의 출처와 적용 결정 | 맵 원본·포맷·타일 해석 변경 |
| [MULTIPLAYER_DESIGN.md](MULTIPLAYER_DESIGN.md) | 멀티플레이의 확정 경계와 목표 계약 | 책임, wire format, 동시성 또는 신뢰 경계 변경 |
| [MULTIPLAYER_STATUS.md](MULTIPLAYER_STATUS.md) | 구현 완료 범위, 알려진 제약과 다음 작업 | 네트워크 구현 단위 완료 |
| [MULTIPLAYER_ROADMAP.md](MULTIPLAYER_ROADMAP.md) | 잔여 MVP 범위, 우선순위와 단계별 완료 조건 | 작업 순서·범위 또는 확정 정책 변경 |
| [NETWORK_LIBRARY_FOLLOWUPS.md](NETWORK_LIBRARY_FOLLOWUPS.md) | 보류한 IOCP·Session·송신 구조의 판단 근거와 재검토 조건 | 해당 후보를 도입하거나 보류 결정을 바꿀 때 |
| [CONSOLE_INPUT_DESIGN.md](CONSOLE_INPUT_DESIGN.md) | 다중 로컬 클라이언트 입력 문제와 후보 | 입력 방식 결정 또는 구현 |

완료된 초기 구현 계획과 단계별 작업 일지는 현재 구조 문서에 중복 기록하지 않는다. 아직 구현되지 않은 항목은 설계 문서의 목표 계약과 상태 문서의 다음 작업으로만 구분해 관리한다.
