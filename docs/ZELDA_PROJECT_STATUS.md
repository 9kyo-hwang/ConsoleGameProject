# Zelda 프로젝트 현재 상태

이 문서는 ZELDA_LIKE_DEVELOPMENT_PLAN.md의 설계를 실제 작업 상태와 구분해 기록한다. 계획 문서는 목표와 계약을 정의하고, 이 문서는 완료된 작업과 직접 확인한 증거를 기록한다.

## 현재 진행

- 현재 단계: 단계 0 - 프로젝트 골격
- 현재 작업: Z1 프로젝트 골격 빌드·실행 검증
- 상태: 감사 통과 - 직접 실행 확인 대기
- 다음 작업: `Binaries\x64\Debug\Z1\Z1.exe`를 직접 실행하고 시작 결과 확인
- 마지막 검증: Z1 audit -Build FAIL=0/WARN=0; 실행 파일·CraftEngine.dll·SoundSystem.dll 생성 확인

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
- [ ] 빌드 출력 디렉터리에서 실행 성공
- [ ] Title/Gameplay/Clear 최소 전환 동작 확인

## 상태 기록 규칙

- 소스가 존재하는 것만으로 완료로 표시하지 않는다.
- 빌드가 필요한 작업은 실제 빌드 결과를 확인한 뒤 완료로 표시한다.
- 실행 동작은 사용자가 직접 확인한 결과를 기록한다.
- 자동 감사는 규칙 위반과 누락 가능성을 찾는 보조 수단이며, 수동 실행 결과를 대체하지 않는다.
- 다음 작업은 한 번에 하나만 기록하고, 완료 기준을 함께 적는다.

## 다음 작업 완료 기준

Z1 프로젝트 골격 작업은 다음을 모두 만족하면 완료로 본다.

- ConsoleGameProject.slnx에 프로젝트가 등록된다.
- Z1의 CraftEngine 빌드 의존성이 연결된다.
- Main.cpp, pch.cpp, pch.h 및 이후 추가되는 소스가 `.vcxproj`와 `.vcxproj.filters`에 등록된다.
- 기존 SokobanGame과 같은 x64/C++20/v145 및 DLL·Content 복사 설정을 갖는다.
- SoundSystem → CraftEngine → Z1 순서로 빌드할 수 있다.
- 실행 파일이 DLL 또는 Config/Content 경로 오류 없이 시작된다.
