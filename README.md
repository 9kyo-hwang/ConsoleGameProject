# [C++로 만드는 게임 엔진 프레임워크](https://www.inflearn.com/course/c-game-engine-framew?cid=341168)

## 학습 목표

- 게임 루프와 고정 프레임 업데이트 구조 이해
- Actor와 Level을 이용한 게임 월드 구성
- Actor와 Component의 책임 및 생명주기 설계
- 입력, 렌더링, 충돌 시스템의 동작 흐름 이해
- 엔진과 게임 코드를 DLL 기반으로 분리하는 방법 학습
- C++ 스마트 포인터와 템플릿 활용
- 커스텀 RTTI와 안전한 타입 처리 구조 구현
- 기존 Actor 중심 코드를 Component 기반 구조로 점진적으로 리팩터링
- Prefab, ActorID, DOD/ECS로 확장되는 구조의 방향 이해

## 주요 기능

- 객체지향 기반 게임 엔진 프레임워크
- 게임 루프와 Actor/Level 기반 월드 관리
- 고정 프레임 기반 업데이트와 `deltaTime` 처리
- 키보드 입력 처리 시스템
- 2차원 벡터와 콘솔 렌더링
- 이중 버퍼링 렌더링
- AABB 기반 충돌 판정
- Engine/Game 프로젝트의 DLL 분리
- 엔진 설정 로드 시스템
- 커스텀 런타임 타입 정보(RTTI)
- Transform, Renderer, Collision 책임을 분리한 Component 구조
- 부모-자식 관계를 표현하는 Transform Scene Graph

## 실습 프로젝트

### 소코반

이동 로직, 맵 구성, 충돌 처리, 게임 상태 관리 흐름 구현

### 비행 슈팅 게임

실시간 입력, 동적 액터 생성, 렌더링, 충돌 처리를 조합해 비행 슈팅 게임 구현

### Component 기반 확장

Transform·렌더링·충돌 책임을 Component로 분리, Scene Graph를 구현하고 현재 구조가 Prefab 및 DOD/ECS 구조로 어떻게 확장될 수 있는지 확인

## 학습 순서

1. 콘솔 게임 엔진 프레임워크 구성
2. Level과 Actor 구조 구현
3. 입력 및 게임 루프 구현
4. Engine/Game DLL 분리
5. 2D 벡터, 렌더링, 이중 버퍼링 구현
6. 설정 로드 시스템과 커스텀 RTTI 구현
7. 소코반 제작
8. 비행 슈팅 게임 제작
9. Actor 구조 복습 및 Component 기반 구조로 확장
10. Transform Scene Graph와 DOD/ECS 확장 방향 학습

