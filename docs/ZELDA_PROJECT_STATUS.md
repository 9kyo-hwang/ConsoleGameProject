# Zelda 프로젝트 현재 상태

이 문서는 ZELDA_LIKE_DEVELOPMENT_PLAN.md의 설계를 실제 작업 상태와 구분해 기록한다. 계획 문서는 목표와 계약을 정의하고, 이 문서는 완료된 작업과 직접 확인한 증거를 기록한다.

## 현재 진행

- 현재 단계: 단계 3 - 한 방 전투 버티컬 슬라이스
- 현재 작업: 전체 Overworld 월드 좌표 기반 이동과 인접 Room 전환 기반 완성
- 상태: 단계 0, 단계 1, 단계 2 완료. 단계 3의 Overworld 배경·전역 통행 판정·플레이어 이동과 기본 인접 Room 전환까지 구현됨
- 다음 작업: 전투에 필요한 플레이어 방향과 공격 연결
- 마지막 검증: Z1 빌드 성공 및 실행에서 BlockingMap 이동 제한과 인접 Room 전환 확인

## 완료 및 검증

- [x] 초기 하네스 문서와 PowerShell 명령 추가
- [x] Git 상태와 기본 저장소 구조를 조회하는 status 명령 추가
- [x] 문서·솔루션·프로젝트 등록을 검사하는 audit 명령 추가
- [x] 읽기 전용 Codex CLI 검토를 호출하는 review 명령 추가
- [x] 실제 프로젝트명 Z1 기준으로 솔루션·소스·빌드 설정 감사 보강
- [ ] review가 Codex 보고서를 제한 시간 안에 반환하는지 확인
- [x] Z1 프로젝트가 솔루션에 등록되고 CraftEngine 의존성이 연결됨
- [x] Z1 소스가 vcxproj와 vcxproj.filters에 등록됨
- [x] Z1에 Binaries/Intermediate, CraftEngine, SoundSystem, Content 복사 설정이 있음
- [x] Z1 Debug|x64 빌드에서 실행 파일 생성 성공
- [x] 빌드 출력 디렉터리에서 실행 성공
- [x] Title/Gameplay/Clear 최소 전환 동작 확인
- [x] CraftEngine N×M Sprite와 기존 문자열 렌더링 경로 구현
- [x] Sprite 투명 셀, 셀별 속성, X/Y 클리핑 및 sorting order 처리
- [x] Z1에서 Sprite 출력과 화면 경계 클리핑을 직접 확인
- [x] BoxComponent를 `size`/`offset` 기반 2D Box로 확장하고 width API 호환 유지
- [x] CollisionSystem에 이전/현재 월드 위치 기반 `SweptBounds` X/Y 판정 적용
- [x] 엔진 변경 후 ShootingGame/SokobanGame/Z1 빌드 및 기존 두 게임 회귀 확인
- [x] Z1 개발 장면에서 1x1/2x2 겹침과 X/Y 분리 수동 확인
- [x] Z1 개발 장면에서 offset이 충돌 영역에 반영되는지 확인
- [x] Z1 개발 장면에서 이전 위치와 현재 위치 사이의 swept 충돌 확인
- [x] `Content/Z1/Maps/Overworld`의 원본 TileId/Blocking 맵 추가
- [x] `OverworldMapLoader`로 256x88 맵 파싱 및 형식 검증
- [x] Room `(7,7)` 및 `(0,0)`의 16x11 추출과 TileId 일치 확인
- [x] `TileSpriteCatalog`으로 현재 Room의 TileId 일부를 문자 Sprite로 매핑
- [x] 논리 타일을 `MapTileSize (5,3)`으로 펼쳐 실제 80x33 Room 배경 Sprite 생성
- [x] Renderer의 Sprite Scale을 제거하고 Sprite를 실제 크기 그대로 1:1 출력
- [x] Renderer의 World/View 변환과 화면 좌표 제출 경로 분리
- [x] Player Actor의 Transform을 전체 Map 기준 월드 좌표로 관리하고 자체 Sprite 크기로 Box 생성
- [x] `BlockingMap`을 `OverworldMapData::CanOccupyWorldRect` 전역 이동 판정에 연결
- [x] Player 월드 좌표로 Room 변경을 감지하고 인접 Room 배경과 View 전환 확인

## 상태 기록 규칙

- 소스가 존재하는 것만으로 완료로 표시하지 않는다.
- 빌드가 필요한 작업은 실제 빌드 결과를 확인한 뒤 완료로 표시한다.
- 실행 동작은 사용자가 직접 확인한 결과를 기록한다.
- 자동 감사는 규칙 위반과 누락 가능성을 찾는 보조 수단이며, 수동 실행 결과를 대체하지 않는다.
- 다음 작업은 한 번에 하나만 기록하고, 완료 기준을 함께 적는다.

## 단계 0 완료 기록

Z1 프로젝트 골격 작업은 다음을 모두 만족하면 완료로 본다.

- ConsoleGameProject.slnx에 프로젝트가 등록된다.
- Z1의 CraftEngine 빌드 의존성이 연결된다.
- Main.cpp, pch.cpp, pch.h 및 이후 추가되는 소스가 `.vcxproj`와 `.vcxproj.filters`에 등록된다.
- 기존 SokobanGame과 같은 x64/C++20/v145 및 DLL·Content 복사 설정을 갖는다.
- SoundSystem → CraftEngine → Z1 순서로 빌드할 수 있다.
- 실행 파일이 DLL 또는 Config/Content 경로 오류 없이 시작된다.

## 단계 1 완료 기록

단계 1 2D 렌더링 작업은 다음을 만족해 완료했다.

- [x] N×M Sprite/셀 데이터를 한 프레임에 합성
- [x] 투명 셀과 양축 클리핑 처리
- [x] 기존 문자열 렌더링 API와 ShootingGame/SokobanGame 출력 유지
- [x] Z1에서 최소 Sprite 출력 장면 직접 확인

## 단계 2 완료 기록

단계 2 2D 충돌 검증은 다음을 확인해 완료했다.

- [x] BoxComponent의 `size`/`offset`과 width 호환 API 구현
- [x] CollisionSystem의 X/Y swept AABB 구현
- [x] 1x1 및 2x2 겹침과 경계 분리를 Z1에서 확인
- [x] X축/Y축으로 분리된 Box가 충돌하지 않음을 확인
- [x] offset이 충돌 영역에 반영됨을 확인
- [x] 이전 위치와 현재 위치 사이를 통과하는 이동 충돌을 확인

SweptBounds 수정 후 Z1 실행 검증까지 완료했다. 기존 두 게임의 최신 엔진 DLL 기준 회귀 플레이는 커밋 전 다시 확인한다.

## 다음 작업 완료 기준

단계 3 한 방 전투 버티컬 슬라이스는 다음을 만족하면 완료로 본다.

- [x] `MapTileSize(5, 3)`과 논리 타일/월드 셀 좌표 변환을 확정
- [x] Overworld 원본 맵을 `Content`에서 읽고 최소 Room 하나를 추출
- [x] TileId를 `TileSpriteCatalog`의 Sprite로 매핑
- [x] 지형 Sprite를 화면에 합성
- [x] 지형 통행 가능 여부를 이동 판정에 연결
