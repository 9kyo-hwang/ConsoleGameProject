# Zelda 맵 데이터 참고 및 적용 결정

## 문서 목적

이 문서는 [8-bit NES Legend of Zelda Map Data](https://inventwithpython.com/blog/8-bit-nes-legend-of-zelda-map-data.html)와 연결된 공개 자료를 CraftEngine 기반 모작에 어떻게 사용할지 정리한다.

페이지의 데이터를 그대로 게임에 넣는 것이 목적이 아니다. 원작의 Room 크기와 타일 단위를 참고하되, 지형 렌더링·충돌·출구·스폰 데이터는 `ZeldaLikeGame`의 콘텐츠 데이터로 관리한다.

## 참고 자료의 범위

- 페이지는 원작 NES 젤다의 **지상 맵**을 탐색하는 Python/Pygame 예제와 타일 맵 데이터를 제공한다.
- 원작 ROM의 압축된 표현을 개별 위치의 타일 ID로 풀어낸 자료다. 원본 ROM의 열 재사용이나 색상 조합 방식을 CraftEngine에서 재현할 필요는 없다.
- 타일 맵은 공백으로 구분된 16진수 ID(`00`, `02`, `3d` 등)로 표현된다.
- 타일 이미지와 ID 대응표도 제공되지만, 모작에는 자체 문자 아트와 허용된 에셋을 사용한다.
- 예제는 걷기만 지원하며 충돌 차단, 몬스터, 아이템, 던전과 레벨은 구현하지 않는다. 따라서 이 자료만으로 완성된 게임 Room 데이터를 얻을 수는 없다.

관련 원본:

- [정리 페이지](https://inventwithpython.com/blog/8-bit-nes-legend-of-zelda-map-data.html)
- [지상 맵 타일 데이터](https://raw.githubusercontent.com/asweigart/nes_zelda_map_data/master/overworld_map/nes_zelda_overworld_tile_map.txt)
- [탐색 예제 코드](https://github.com/asweigart/nes_zelda_map_data/blob/master/nesZeldaWalkingTour.py)

## 원작 지상 맵에서 채택할 구조

| 원작 자료의 단위 | 크기 | 프로젝트에서의 의미 |
| --- | ---: | --- |
| 지상 맵 | `16 × 8 Room` | `Overworld` Area의 Room 좌표 참고 |
| 전체 지상 타일 배열 | `256 × 88` | 전역 맵 자료를 사용할 때의 크기 |
| Room 하나 | `16 × 11` 논리 타일 | `RoomDefinition`의 기본 격자 크기 |
| 원작 타일 | `16 × 16` NES 픽셀 | 논리 타일 하나의 출처 단위. 콘솔 셀 개수가 아님 |

원작 화면은 마지막 타일 행의 위쪽 절반만 보여준다. CraftEngine의 콘솔 셀은 픽셀 단위가 아니므로 첫 구현에서는 `16 × 11` 논리 격자를 그대로 유지하고, 필요하면 Room viewport에서 하단을 클리핑한다.

페이지 본문과 예제 코드에는 전체 높이를 `1344`와 `1408` 픽셀로 각각 표현한 차이가 있다. 이는 마지막 행의 부분 표시를 포함하는지 여부의 차이로 보이며, 프로젝트의 논리 Room 크기 `16 × 11`을 정하는 데 영향을 주지 않는다.

## `2 × 1` 콘솔 셀 정책의 정확한 의미

앞서 계획에서 말한 `2 × 1`은 페이지의 `16 × 16` 픽셀 타일을 그대로 변환한 값이 아니다.

CraftEngine의 현재 백엔드는 `CHAR_INFO` 하나를 콘솔 셀 하나로 출력한다. 일반적인 콘솔 글꼴의 셀은 화면에서 가로보다 세로가 긴 편이므로, 정사각형에 가까운 논리 타일을 표현하려면 다음과 같은 변환을 사용한다.

```text
원작의 16 × 16 픽셀 타일
        ↓  (프로젝트의 논리 타일 1개)
콘솔 셀 2열 × 1행
```

따라서 다음을 구분한다.

- `N × M Sprite`: Renderer가 지원해야 하는 임의 크기의 콘솔 셀 배열
- `2 × 1`: 논리 타일 하나의 초기 콘솔 셀 footprint
- `16 × 16`: 원작 자료의 NES 픽셀 단위

플레이어, 검, 보스처럼 한 타일보다 큰 대상은 `2 × 1`로 제한되지 않고 여러 콘솔 셀을 사용하는 `N × M Sprite`가 된다. Renderer는 Zelda의 타일 크기를 알지 않고 콘솔 셀 단위만 처리한다. `2 × 1`은 Zelda 콘텐츠의 `TileMetrics`에서 관리한다.

첫 구현의 결정은 다음과 같다.

```cpp
struct TileMetrics
{
    int consoleWidthPerTile = 2;
    int consoleHeightPerTile = 1;
};
```

이 값은 원작 픽셀 데이터를 따르는 고정 규격이 아니라 현재 `60 × 25` 콘솔 화면과 문자 아트의 가독성을 고려한 초기 정책이다. 화면 크기나 시각 테스트 결과가 바뀌면 Room 파일을 바꾸지 않고 `TileMetrics`와 좌표 변환만 조정한다.

## Room 파일 포맷 결정

### 기본 포맷

MVP에서는 Room 하나를 파일 하나로 관리한다.

```text
Content/ZeldaLike/Rooms/Overworld_00_00.txt
```

파일에는 **지형 타일 ID만** 공백으로 구분해 저장한다.

- 행 수: `11`
- 행마다 토큰 수: `16`
- 토큰: 16진수 `TileId` (`00`~`ff` 범위)
- 주석, 스폰, 출구 지시는 기본 포맷에 넣지 않는다.

이렇게 하면 한 토큰이 콘솔 문자 하나나 Actor 하나라는 오해가 생기지 않는다. 토큰은 논리 타일 하나이며, 렌더링 시 `TileDefinition`을 거쳐 `2 × 1` 이상의 콘솔 셀로 확장된다.

페이지의 `256 × 88` 전역 맵 파일을 직접 활용하고 싶을 때는 별도의 변환 도구 또는 선택적 로더가 `(roomX * 16, roomY * 11)` 위치에서 `16 × 11` 영역을 추출하도록 한다. MVP의 런타임 `RoomLoader`는 전역 맵을 알 필요 없이 Room 파일만 읽는다.

### Room 외부 메타데이터

출구, 진입 위치, 적 스폰, 보스 여부는 타일 ID와 분리한다.

```text
RoomDefinition
  - areaId / roomId
  - width = 16, height = 11
  - tileIds[11][16]
  - exits[4]
  - entryPositions[4]
  - actorSpawns
  - boss flag
```

초기에는 `RoomCatalog` 또는 C++ 초기화 코드로 메타데이터를 제공해도 된다. Room 수가 늘어나면 `Room_*.meta` 같은 별도 텍스트 포맷으로 옮긴다. 지형 토큰과 엔티티 토큰을 한 파일에 섞는 방식은 16진수 타일 포맷과 충돌할 수 있으므로 기본 선택에서 제외한다.

## 타일 로드와 렌더링 파이프라인

```text
RoomLoader
  → 16 × 11 TileId 검증
  → RoomDefinition 생성
  → TileDefinition 조회
  → 타일 Sprite를 Room 배경으로 합성
  → 타일 속성으로 통행/충돌 질의
  → 동적 스폰만 Actor 생성
```

`TileDefinition`은 시각 정보와 게임플레이 속성을 함께 제공하되, 정적 지형을 Actor로 만들지는 않는다.

```cpp
struct TileDefinition
{
    TileId id;
    Sprite sprite;       // 콘솔 셀 배열
    bool walkable;
    bool blocksProjectiles;
    bool isHazard;
    bool isExit;
};
```

필요해지면 `CollisionMap`을 별도로 두어 시각 타일 하나가 여러 충돌 셀을 차지하거나, 같은 Sprite가 Room에 따라 다른 통행 속성을 갖는 경우를 처리한다. 첫 버전은 `TileDefinition::walkable`로 시작한다.

플레이어와 적의 위치·충돌은 최종 콘솔 셀 단위를 사용한다. 논리 Room 좌표를 화면에 배치할 때는 다음 변환을 한 곳에서 수행한다.

```text
logicalTile(x, y)
  → consolePosition(x * 2, y)
```

이 변환을 `RoomRenderer`와 Zelda 콘텐츠의 이동/충돌 보조 함수가 공유하면, 나중에 `TileMetrics`를 변경해도 Room 파일과 게임 규칙을 다시 작성할 필요가 없다.

## Area와 Room에 대한 적용

이 자료가 다루는 `16 × 8 Room` 격자는 `Overworld`라는 하나의 `Area`로 볼 수 있다.

- `Area`: Overworld, Cave, Dungeon처럼 여러 Room과 연결 규칙을 묶는 콘텐츠 그룹
- `Room`: 한 번에 표시되는 고정 화면 하나
- `GameplayLevel`: 현재 Area/Room을 관리하고 Room 전환과 Actor 수명을 조정

동굴과 던전은 이 외부 지상 맵 자료에 포함되지 않으므로 자체 Room 파일과 `RoomCatalog` 메타데이터를 만든다. 그렇다고 Area마다 Level을 만들 필요는 없다.

## 구현 범위와 보류 사항

이 자료를 도입해도 다음 엔진 변경은 그대로 필요하다.

- Renderer의 2차원 셀 Sprite, 투명도, 양축 클리핑
- 2차원 BoxComponent와 지형 통행 질의의 콘텐츠 구현
- Room 배경과 동적 Actor의 분리

반대로 다음은 현재 범위에 추가하지 않는다.

- 원작 ROM의 압축 저장 방식 재현
- 픽셀 단위 NES 렌더러
- 전체 128개 지상 Room의 완전한 이식
- 외부 타일 PNG의 직접 배포
- 페이지 예제에 없는 몬스터·아이템·던전 데이터의 추정 생성

외부 자료의 타일 데이터나 이미지를 실제 저장소에 복사하기 전에는 해당 저장소의 사용 조건을 별도로 확인한다. 프로젝트의 기본 구현은 자체 제작 타일과 데이터로 진행한다.
