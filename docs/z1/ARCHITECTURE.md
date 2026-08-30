# Z1 아키텍처

## 문서 범위

이 문서는 Z1 싱글플레이 콘텐츠의 현재 실행 구조와 데이터 책임을 설명한다. 공용 엔진 계약은 [솔루션 아키텍처](../ARCHITECTURE.md), 네트워크 목표 구조는 [멀티플레이 설계](MULTIPLAYER_DESIGN.md), 구현 진척은 [멀티플레이 현황](MULTIPLAYER_STATUS.md)에서 관리한다.

## 게임 상태와 Level

`Z1::Game`은 다음 상태에 대응하는 Level을 보유한다.

```text
Title
  └─ 새 게임 → Overworld
                  ├─ SwordCave ↔ Overworld
                  ├─ Dungeon1 ↔ Overworld
                  ├─ HP 0 → GameOver → Title
                  └─ 보스·트라이포스 → Clear → Title

Development                  수동 엔진 기능 확인 장면
```

새 게임을 시작하면 Overworld, SwordCave, Dungeon1 Level을 다시 만들고 Player 상태를 최대 HP 20, 검 미소지로 초기화한다. Level 전환 시 `Game`이 HP와 검 보유 상태를 보관하고 새 Level의 Player에 복원한다.

## 좌표와 Room

- Player와 Enemy Transform은 전체 맵 기준 월드 콘솔 셀 좌표를 사용한다.
- 오버월드는 256×88 논리 타일이며 한 Room은 16×11 타일이다.
- 논리 타일 하나는 현재 10×5 콘솔 셀 Sprite로 표현한다.
- Room은 별도 소유 객체가 아니라 전체 맵에서 현재 화면에 표시할 논리 구간이다.
- Level은 Room의 월드 원점을 Renderer View에 설정하고 Map 타입에 배경 Sprite 합성을 요청한다.
- Room 경계를 넘을 때 Player Box 전체가 새 Room 안에 들어오도록 위치를 보정한다.

맵 원본과 타일 해석은 [맵 데이터](MAP_DATA.md)를 따른다.

## Map과 Level의 책임

| 타입 | 책임 |
| --- | --- |
| `OverworldMap` | TileMap/BlockingMap 파싱·검증, TileId 보관, Engine Tilemap 구성과 Room Sprite 합성 |
| `CaveMap` | 동굴 문자 맵 파싱, 검과 출구 표식 위치 제공, Tilemap 구성 |
| `DungeonMap` | Dungeon 1 문자 맵 파싱, 보스·하트·트라이포스 표식 제공, Tilemap 구성 |
| `OverworldLevel` | 현재 Room과 View, Player·Enemy·Projectile 수명, 이동·전투·입구 전환 |
| `CaveLevel` | 검 획득, Player 상태와 오버월드 복귀 |
| `DungeonLevel` | Room 전환, 일반 적과 Aquamentus, 보상·클리어·출구 처리 |

정적 지형은 타일마다 Actor를 만들지 않고 하나의 Room 배경 Sprite와 Tilemap의 blocked 데이터로 처리한다. 동적 행동이나 수명이 필요한 Player, Enemy, Projectile, 공격과 효과만 Actor로 생성한다.

## Actor와 전투

`Pawn`은 Player와 Enemy가 공유하는 HP, facing, 정수 Transform에 반영하기 전의 이동 누산, 피해·사망·넉백과 피격 무적 상태를 담당한다.

- Player는 방향키로 이동하고 마지막 facing을 유지한다.
- 검 보유 상태에서 `A`를 누르면 Player에 붙는 `SwordAttack`을 만들고, 최대 HP에서는 검기를 발사한다.
- Octorok은 방향을 바꾸며 돌을 발사한다.
- Moblin은 Player를 추적하고 창을 발사한다.
- Tektite는 대기와 대각선 도약을 반복하며 지형 blocked 판정은 무시하지만 Player 접촉 피해는 적용한다.
- Aquamentus는 수평 이동과 세 갈래 화염구 공격을 사용한다.
- 적 투사체는 Player의 facing과 방패 방향이 맞으면 차단된다.

충돌 콜백은 피해 후보를 알리고, 실제 이동 가능 여부와 Room·지형 정책은 Level이 판정한다. 공격 하나가 같은 대상에 중복 피해를 주지 않도록 공격자와 피해 원인을 함께 전달한다.

## 수명과 Room 전환

Overworld의 시작 Room에는 적이 없다. 다른 접근 가능 Room은 Room 좌표와 월드 시드로 결정적인 스폰 계획을 만들며, Room을 나가면 이전 Enemy와 Projectile을 파괴하고 새 Room의 Actor를 생성한다.

동굴과 던전은 별도 Level이다. 입구 전환은 Player Box가 지정 영역에 닿았을 때 수행한다. Dungeon 1은 검을 가진 상태에서만 진입할 수 있고, 보스 처치 뒤 하트는 HP를 전부 회복하며 트라이포스는 팬파레 후 Clear Level로 전환한다.

## 네트워크 연결 지점

`Game`은 `NetworkClient`를 소유하고 localhost `127.0.0.1:7777` 연결을 시도한다. network thread는 TCP 송수신과 packet parsing만 수행하고, 받은 메시지는 queue로 main thread에 넘긴다. `OverworldLevel`이 queue를 소비해 Snapshot을 적용한다.

현재 원격 playerId는 `NetworkPlayer` Actor로 생성·갱신·제거한다. 로컬 Player는 아직 기존 싱글플레이 Actor와 판정을 사용하므로 서버 권위형 전환이 완료된 상태가 아니다. 정확한 완료 범위와 다음 작업은 [멀티플레이 현황](MULTIPLAYER_STATUS.md)을 따른다.
