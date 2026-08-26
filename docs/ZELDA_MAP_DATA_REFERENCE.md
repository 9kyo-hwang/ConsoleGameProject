# Zelda 맵 데이터 참고 및 적용 결정

## 문서 목적

이 문서는 [8-bit NES Legend of Zelda Map Data](https://inventwithpython.com/blog/8-bit-nes-legend-of-zelda-map-data.html)와 연결된 공개 자료를 CraftEngine 기반 모작에 어떻게 사용할지 정리한다.

페이지의 데이터를 그대로 게임에 넣는 것이 목적이 아니다. 원작의 Room 크기와 타일 단위를 참고하되, 지형 렌더링·충돌·출구·스폰 데이터는 `Z1`의 콘텐츠 데이터로 관리한다.

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
| Room 하나 | `16 × 11` 논리 타일 | 전체 Map에서 현재 화면에 표시할 구간 크기 |
| 원작 타일 | `16 × 16` NES 픽셀 | 논리 타일 하나의 출처 단위. 콘솔 셀 개수가 아님 |

원작 화면은 마지막 타일 행의 위쪽 절반만 보여준다. CraftEngine의 콘솔 셀은 픽셀 단위가 아니므로 첫 구현에서는 `16 × 11` 논리 격자를 그대로 유지하고, 필요하면 Room viewport에서 하단을 클리핑한다.

페이지 본문과 예제 코드에는 전체 높이를 `1344`와 `1408` 픽셀로 각각 표현한 차이가 있다. 이는 마지막 행의 부분 표시를 포함하는지 여부의 차이로 보이며, 프로젝트의 논리 Room 크기 `16 × 11`을 정하는 데 영향을 주지 않는다.

## Map 타일의 콘솔 셀 크기

초기 문서의 `2 × 1`과 이후의 `5 × 3`은 임시 표시값이었고 현재 Overworld는 논리 타일 하나를 콘솔 셀 `10 × 5`로 표시한다. 이 값은 원작의 `16 × 16` NES 픽셀을 직접 변환한 것이 아니라 현재 콘솔 글꼴과 Room 표시 크기에 맞춘 Z1 콘텐츠 정책이다.

```cpp
const Craft::Vector2 TileCellSize(10, 5);
```

다음 단위를 구분한다.

- `16 × 16`: 원작 자료의 NES 픽셀 단위
- `16 × 11`: Room 하나의 논리 타일 격자
- `10 × 5`: Overworld 논리 타일 하나가 차지하는 월드·콘솔 셀 크기
- `160 × 55`: 현재 Room 배경 Sprite의 실제 콘솔 셀 크기
- `N × M Sprite`: Actor나 배경이 실제로 차지하는 콘솔 셀 배열

Renderer는 Z1의 타일 크기를 모르며 Sprite를 저장된 크기 그대로 1:1로 출력한다. `TileCellSize`는 Z1 Map이 Engine `Tilemap`을 구성할 때 전달하며, Tilemap의 Room 배경 합성·좌표 변환·통행 판정에서 사용한다. Player, 검, 적과 무기는 자체 Sprite와 Box 크기를 가지므로 Map 타일 크기에 종속되지 않는다. `Config/Setting.txt`의 `width`와 `height`도 NES 픽셀 해상도가 아니라 콘솔 셀 버퍼 크기다.

## Room 파일 포맷 결정

### Overworld 원본 포맷

```text
Content/Z1/Maps/Overworld/TileMap.txt
Content/Z1/Maps/Overworld/BlockingMap.txt
```

Overworld는 원본 전체 맵을 그대로 보관한다.

- TileId 맵: `88`행 × `256`토큰
- Blocking 맵: `88`행 × `256`문자
- TileId 맵 토큰: 16진수 `TileId` (`00`~`ff` 범위)
- Blocking 맵 문자: `.`은 이동 가능, `X`는 이동 불가
- 주석, 스폰, 출구 지시는 원본 맵 파일에 넣지 않는다.

토큰 하나는 논리 타일 하나다. 현재 구현은 `OverworldMap`이 소유한 `unordered_map<TileId, shared_ptr<const Sprite>>`에서 완성된 10×5 Tile Sprite를 찾고, BlockingMap의 값과 함께 Engine `Tilemap`에 설정한다. 별도 Catalog 타입은 두지 않으며 Tilemap은 지정한 논리 구간의 각 Tile Sprite를 복사해 Room 배경을 합성한다.

`OverworldMap`은 TileMap 파일을 TileId 배열로, BlockingMap 파일을 임시 blocked 배열로 각각 검증한다. 두 파싱이 모두 성공하면 같은 좌표의 TileId를 Z1 Tile Sprite로 해석하고 blocked 값과 함께 Engine `Tilemap`에 설정한다. TileId 배열은 입구 판정 등 콘텐츠 규칙을 위해 별도로 유지한다. Room 전환 시 데이터를 별도 Room 객체로 복사하지 않고 `(roomX * 16, roomY * 11)`에서 시작하는 `16 × 11` 구간을 전체 Tilemap에서 합성한다. 동굴과 던전은 별도 문자 파일을 사용하지만 같은 Tilemap 구성·합성 방식을 따른다.

### Room 외부 메타데이터

출구, 진입 위치, 적 스폰, 보스 여부는 타일 ID와 분리한다.

현재 인접 Overworld Room 이동은 전체 Map에서 연속된 월드 좌표로 판정하므로 별도 출구 정보가 필요 없다. Player의 후보 월드 좌표가 다른 Room 영역에 들어가면 현재 Room 배경과 View만 교체한다.

동굴 입구, 특수 워프, 적 스폰과 보스처럼 단순 인접 이동으로 표현할 수 없는 정보가 실제로 필요해지면 TileId·BlockingMap과 분리된 Room 메타데이터를 추가한다. 초기에는 C++ 초기화 데이터로 시작하고 양이 늘어날 때 별도 파일 포맷을 도입한다.

## 타일 로드와 렌더링 파이프라인

```text
OverworldMap
  → 256 × 88 TileId/Blocking 맵을 각각 검증
  → TileId 원본은 Z1 규칙용 배열에 보관
  → TileId를 10 × 5 Sprite로, Blocking 값을 blocked로 해석
  → Engine Tilemap의 각 셀에 Tile 설정
  → 현재 Room의 16 × 11 구간을 160 × 55 Sprite로 합성
  → Tilemap에 Actor의 월드 Box 통행 여부 질의
  → 월드 좌표가 속한 Room이 바뀌면 배경과 View 교체
```

시각 정보와 통행 원본은 분리한다. `OverworldMap`의 TileId-Sprite map은 시각 정보만 정하고, 별도 BlockingMap이 각 Engine Tile의 blocked 값을 정한다. 따라서 같은 TileId라도 BlockingMap 데이터에 따라 다른 통행 결과를 가질 수 있고, Sprite map이 게임플레이 속성을 재정의하지 않는다.

Actor의 Transform과 Box는 전체 Map 기준 월드 셀 단위를 사용한다. 좌표 변환은 다음 두 단계다.

```text
mapTile(x, y)
  → worldCell(x * TileCellSize.x, y * TileCellSize.y)

worldCell
  → screenCell = worldCell - viewWorldOrigin + viewScreenOrigin
```

`OverworldLevel`은 현재 Room의 월드 좌상단을 View의 `worldOrigin`으로, HUD 아래 `(0, 3)`을 `screenOrigin`으로 설정한다. Room이 바뀌어도 Actor Transform은 전역 월드 좌표를 그대로 유지한다.

## Area와 Room에 대한 적용

이 자료가 다루는 `16 × 8 Room` 격자는 `Overworld`라는 하나의 `Area`로 볼 수 있다.

- `Area`: Overworld, Cave, Dungeon처럼 여러 Room과 연결 규칙을 묶는 콘텐츠 그룹
- `Room`: 한 번에 표시되는 고정 화면 하나
- `OverworldLevel`: 현재 Area/Room을 관리하고 Room 전환과 Actor 수명을 조정

동굴과 던전은 이 외부 지상 맵 자료에 포함되지 않으므로 각각 자체 문자 Map 파일을 사용한다. Z1의 `CaveMap`과 `DungeonMap`이 문자 원본과 콘텐츠 표식을 보관하고, 지형 시각·blocked 상태는 내부 Engine Tilemap에 설정한다.

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
