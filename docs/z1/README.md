# Z1 문서 안내

## 범위

이 디렉터리는 `Z1`, `Z1Server`, `Z1Shared`와 Z1이 사용하는 네트워크 경계에만 적용되는 문서를 모은다. 전체 솔루션의 공통 엔진 구조와 빌드는 각각 [솔루션 아키텍처](../ARCHITECTURE.md), [빌드와 개발](../DEVELOPMENT.md)을 기준으로 한다.

## 현재 프로젝트

Z1은 Zelda형 고정 화면 콘텐츠를 CraftEngine 위에 구현한 프로젝트다. Title에서 `LocalPlay`를 선택하면 오버월드의 검 동굴을 거쳐 Dungeon 1의 보스를 처치하고 클리어하는 싱글플레이 흐름을 실행한다.

`MultiPlay`는 localhost의 Z1Server에 연결해 별도 `NetworkOverworldLevel`에서 실행된다. 클라이언트는 이동·공격 의도만 보내고, 서버가 20Hz simulation에서 Player 이동과 지형 충돌, Enemy의 A* 추적, Projectile, HP·사망과 일반 검 판정을 처리한다. 서버 Snapshot과 CombatEvent를 받은 각 클라이언트는 `MyPlayer`, 원격 `NetworkPlayer`, Enemy, Projectile과 검 효과를 main thread에서 표현한다. 연결 종료나 local Player 사망 시 네트워크 상태를 정리하고 Title로 복귀한다.

같은 PC의 여러 클라이언트는 CraftEngine의 콘솔 입력 이벤트를 각 창에서 소비하므로 포커스된 콘솔의 Player만 독립적으로 조작된다. 현재 멀티플레이 범위는 Overworld에 한정되며 Cave와 Dungeon은 Local Play에서만 지원한다.

## 문서 역할

| 문서 | 역할 | 갱신 시점 |
| --- | --- | --- |
| [ARCHITECTURE.md](ARCHITECTURE.md) | 현재 싱글·멀티플레이 구조와 Z1 내부 책임 | Level, Actor, 맵 또는 상태 흐름 변경 |
| [NETWORK_ARCHITECTURE.md](NETWORK_ARCHITECTURE.md) | 현재 네트워크 책임, wire format, 동시성과 서버 권위 경계 | protocol, Session, thread 또는 권위 규칙 변경 |
| [TESTING.md](TESTING.md) | Z1 빌드·실행과 수동/스크립트 검증 | 관찰 가능한 동작이나 검증 방법 변경 |
| [MAP_DATA.md](MAP_DATA.md) | 외부 맵 자료의 출처와 적용 결정 | 맵 원본·포맷·타일 해석 변경 |

설계·구현 과정의 작업 기록과 초안은 로컬에서 관리하며 저장소의 현재 계약으로 사용하지 않는다. 아직 구현되지 않은 작업은 [GitHub Issues](https://github.com/9kyo-hwang/ConsoleGameProject/issues)에서 관리하고, 구현이 완료되어 현재 구조가 바뀌면 위 기준 문서를 갱신한다.
