# Z1 문서 안내

## 현재 프로젝트

Z1은 NES 기기를 대상으로 출시된 초대 젤다의 전설 컨텐츠를 CraftEngine 위에서 구현한 모작 프로젝트입니다. Title에서 `LocalPlay`를 선택하면 오버월드의 검 동굴을 거쳐 Dungeon 1의 보스를 처치하고 클리어하는 싱글플레이 흐름을 실행합니다.

`MultiPlay`는 localhost의 Z1Server에 연결해 별도 `NetworkOverworldLevel`에서 실행됩니다. 클라이언트는 이동·공격 의도만 보내고, 서버가 수행하는 시뮬레이션에서 Player 이동과 지형 충돌, Enemy의 A* 추적, Projectile, HP·사망과 일반 검 판정을 처리합니다. 서버 Snapshot과 CombatEvent를 받은 각 클라이언트는 `MyPlayer`, 원격 `NetworkPlayer`, Enemy, Projectile과 검 효과를 메인 스레드에서 표현합니다. 연결이 해제되거나 내 플레이어 사망 시 네트워크 상태를 정리하고 Title로 복귀하는 흐름을 갖습니다.

## 문서 역할

| 문서 | 역할 | 갱신 시점 |
| --- | --- | --- |
| [ARCHITECTURE.md](ARCHITECTURE.md) | 현재 싱글·멀티플레이 구조와 Z1 내부 책임 | Level, Actor, 맵 또는 상태 흐름 변경 |
| [NETWORK_ARCHITECTURE.md](NETWORK_ARCHITECTURE.md) | 현재 네트워크 책임, wire format, 동시성과 서버 권위 경계 | protocol, Session, thread 또는 권위 규칙 변경 |
| [TESTING.md](TESTING.md) | Z1 빌드·실행과 수동/스크립트 검증 | 관찰 가능한 동작이나 검증 방법 변경 |
| [MAP_DATA.md](MAP_DATA.md) | 외부 맵 자료의 출처와 적용 결정 | 맵 원본·포맷·타일 해석 변경 |
