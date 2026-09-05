# 콘솔 입력 시스템 개선 검토

## 문서 목적

이 문서는 CraftEngine의 현재 키 입력 방식이 같은 PC에서 실행한 여러 Z1 클라이언트에 미치는 영향과 개선 후보를 기록한다. Z1 protocol과 서버 입력 검증은 [멀티플레이 설계](MULTIPLAYER_DESIGN.md)에서 다루고, 여기서는 클라이언트가 어느 키 입력을 자신의 입력으로 받아들일지만 다룬다.

현재 결론 및 진행 상태는 다음과 같다.

- `MyPlayer`/`NetworkPlayer` 분리로 한 클라이언트 내의 Actor별 입력 책임을 해결했다.
- `CraftEngine::Input`을 콘솔 입력 버퍼의 `KEY_EVENT_RECORD` 및 `FOCUS_EVENT` 소비(후보 A)로 전환하여, 동일 PC에서 여러 Z1을 실행해도 포커스된 콘솔 창에만 입력이 전달되도록 분리를 완료했다.
- CraftEngine의 기존 `GetKey`/`GetKeyDown`/`GetKeyUp` 공개 API는 그대로 유지된다.

## 현재 Z1에서 확인된 현상

CraftEngine의 `Input::ProcessInput`은 매 frame 가상 키 0~255를 다음처럼 polling한다.

```cpp
_keyStates[key].isKeyDown =
    (::GetAsyncKeyState(key) & 0x8000) != 0;
```

`GetAsyncKeyState`의 최상위 bit는 함수 호출 시점에 해당 물리 키가 눌려 있는지를 나타낸다. 이 값은 특정 Z1 Actor나 console 입력 buffer에 귀속된 상태가 아니다. 같은 사용자 desktop에서 Z1 프로세스 두 개가 같은 frame 구간에 방향키를 polling하면 둘 다 눌림 상태를 볼 수 있다. 하위 bit의 "이전 호출 뒤 눌림" 정보는 선점형 multitasking 환경에서 다른 application이 먼저 관찰할 수 있어 신뢰하면 안 되지만, 현재 코드는 최상위 bit만 사용하므로 이 하위 bit 문제와는 무관하다. 자세한 API 계약은 Microsoft의 [`GetAsyncKeyState`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getasynckeystate) 문서를 따른다.

현재 Z1 두 개를 실행한 검증에서는 방향키 한 번에 서버 로그가 다음처럼 기록됐다.

```text
player=1, sequence=1, direction=Left
player=2, sequence=1, direction=Left
```

한 클라이언트 안의 원격 `NetworkPlayer`가 키를 읽은 결과가 아니다. 각 프로세스의 `OverworldLevel::SendNetworkInput`이 같은 물리 키를 감지해 자기 Session의 입력을 하나씩 보낸 결과다. 서버가 두 Player를 같은 방향으로 이동시키고 양쪽 snapshot에 그 결과를 담으므로 화면에서는 로컬·원격 Player가 함께 움직이는 것처럼 보인다.

`MyPlayer`가 도입되면 입력 polling 위치는 `OverworldLevel`에서 각 프로세스의 `MyPlayer` 하나로 이동한다. 이 변경으로 한 프로세스 안의 원격 `NetworkPlayer`가 입력을 읽는 일은 막지만, 두 프로세스의 `MyPlayer`가 같은 `GetAsyncKeyState` 결과를 읽는 현상은 그대로 남는다.

## GUI message 입력과의 차이

일반적인 Win32 GUI 게임 클라이언트는 다음처럼 창의 message queue와 로컬 플레이어의 입력 책임을 함께 사용한다.

1. `Game`이 Win32 GUI `HWND`와 message loop를 가진다.
2. `InputManager::Update`는 `GetKeyboardState`로 256개 가상 키 상태를 복사한다.
3. `MyPlayer::TickIdle`만 `TickInput`을 호출한다.
4. 원격 `Player::TickIdle`은 입력을 읽지 않는다.

`GetKeyboardState`가 반환하는 상태는 호출 thread가 keyboard message를 message queue에서 제거하면서 갱신된다. 하드웨어의 비동기 현재 상태를 직접 묻는 `GetAsyncKeyState`와 다르다. 자세한 계약은 Microsoft의 [`GetKeyboardState`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeyboardstate)와 [`GetKeyState`](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getkeystate) 설명을 기준으로 한다.

이 구조에서는 두 경계가 동시에 작동한다.

```text
Win32 focus와 message queue
  → 활성 client thread의 keyboard state

MyPlayer만 TickInput
  → 한 client 안에서는 자기 Player만 입력 처리
```

Z1은 Win32 GUI message window가 아니라 Windows console/Windows Terminal에서 실행된다. 따라서 `GetKeyboardState` 호출로 바꾸기만 하면 GUI와 같은 focus 경계가 자동으로 생긴다고 가정하면 안 된다. 실제 Z1 하나와 입력을 생성하지 않는 dummy client 조합에서는 여러 실제 client가 같은 키를 polling하는 문제도 드러나지 않으므로, 입력 구현 자체를 별도로 검증해야 한다.

## 개선 후보

### 후보 A: console input buffer의 KEY_EVENT_RECORD 사용

`STD_INPUT_HANDLE`이 가리키는 console input buffer를 `ReadConsoleInputW`로 읽고, `INPUT_RECORD::Event.KeyEvent`의 `KEY_EVENT_RECORD`를 처리한다. Console은 keyboard focus를 가질 때 key press/release를 자신의 input buffer에 event record로 넣는다. 관련 계약은 Microsoft의 [Console Input Buffer](https://learn.microsoft.com/en-us/windows/console/console-input-buffer), [`ReadConsoleInput`](https://learn.microsoft.com/en-us/windows/console/readconsoleinput), [Low-Level Console Input Functions](https://learn.microsoft.com/en-us/windows/console/low-level-console-input-functions)를 기준으로 한다.

장점:

- press와 release가 각각 `bKeyDown` event로 전달되어 기존 `GetKeyDown`/`GetKeyUp` 의미를 만들 수 있다.
- `wVirtualKeyCode`를 사용해 현재 가상 키 기반 API와 연결하기 쉽다.
- 각 console/pseudoconsole input stream으로 전달된 event를 사용하므로 여러 Z1 instance의 입력을 분리할 가능성이 가장 높다.
- Z1뿐 아니라 같은 CraftEngine을 사용하는 ShootingGame과 SokobanGame에도 같은 public Input API를 유지할 수 있다.

주의점:

- `ReadConsoleInputW`는 event가 없으면 block하므로 frame loop에서 무조건 호출하면 안 된다. `GetNumberOfConsoleInputEvents`로 unread count를 확인한 뒤 준비된 record만 drain해야 한다.
- key auto-repeat은 같은 key-down event를 여러 번 만들 수 있다. 이미 down 상태인 key의 반복 event는 `GetKeyDown`을 다시 발생시키지 않아야 한다.
- 한 frame 사이에 press와 release가 모두 들어올 수 있다. 최종 down 상태만 저장하면 짧은 tap을 잃으므로 `pressedThisFrame`, `releasedThisFrame`, `isDown`을 따로 유지하는 편이 안전하다.
- stdin이 pipe/file로 redirect됐거나 console handle이 아니면 `GetConsoleMode`가 실패할 수 있다. 이 경우 전역 `GetAsyncKeyState`로 조용히 fallback하면 같은 문제가 되살아나므로 입력 비활성화와 진단 로그 중 하나를 명시적으로 선택한다.
- console mode를 바꾼다면 기존 mode를 저장하고 종료 시 복원해야 한다. `ENABLE_PROCESSED_INPUT`, Quick Edit, `CTRL+C` 처리에 미치는 영향도 별도로 확인한다.
- Microsoft는 새 cross-platform terminal 제품에 classic low-level console API보다 virtual terminal 방식을 권장한다. 다만 현재 저장소는 Windows 전용 Win32 console renderer이고 key-up/held 상태가 필요하므로, 범위를 제한한 `KEY_EVENT_RECORD` 적용은 실용적인 선택이다. 이 trade-off는 Microsoft의 [Classic Console APIs versus Virtual Terminal Sequences](https://learn.microsoft.com/en-us/windows/console/classic-vs-vt) 설명과 함께 판단한다.

현재 프로젝트에는 이 후보를 우선 권장한다. 구현 위치는 Z1이 아니라 `CraftEngine::Input` 내부이며, 외부 API는 다음처럼 유지한다.

```cpp
bool GetKey(int keyCode) const;
bool GetKeyDown(int keyCode) const;
bool GetKeyUp(int keyCode) const;
```

내부 상태는 다음 형태가 적합하다.

```cpp
struct KeyState
{
    bool isDown = false;
    bool pressedThisFrame = false;
    bool releasedThisFrame = false;
};
```

frame 시작에 edge 두 값을 지우고 console event를 drain한다. `bKeyDown=true`이고 이전 `isDown=false`일 때만 `pressedThisFrame`, 반대 전환일 때만 `releasedThisFrame`을 세운다. `isDown`은 다음 event까지 유지한다.

### 후보 B: 현재 polling에 foreground-window 조건 추가

현재 `GetAsyncKeyState`를 유지하되 자기 console host가 foreground일 때만 polling하는 방식이다.

장점:

- 변경량이 작다.
- 고전적인 별도 `conhost.exe` window에서는 동작을 확인하기 쉽다.

단점:

- Windows Terminal과 pseudoconsole에서는 화면에 보이는 foreground window가 terminal host 소유이고, client process가 얻는 console window handle과 일치하지 않을 수 있다.
- tab과 pane 중 어느 것이 실제 입력 대상인지 client가 `GetForegroundWindow` 비교만으로 안정적으로 판단하기 어렵다.
- focus를 잃는 순간 `None`/key-up을 만들지 못하면 이전 방향이 눌린 상태로 남을 수 있다.

Windows Terminal을 기본 실행 환경으로 쓰는 현재 저장소에는 임시 진단 외의 최종 해법으로 권장하지 않는다.

### 후보 C: Win32 GUI input window와 GetKeyboardState 도입

Rookiss처럼 각 client가 실제 `HWND`와 message loop를 소유하고 `GetKeyboardState` 또는 `WM_KEYDOWN`/`WM_KEYUP`으로 입력을 관리한다.

장점:

- focus와 입력 대상이 Win32 window 단위로 명확하다.
- Rookiss 구조와 가장 유사하고 Raw Input 등으로 확장하기 쉽다.

단점:

- Z1의 console renderer와 실행 모델에 비해 범위가 지나치게 크다.
- 입력 문제 하나를 해결하기 위해 별도 window 생성, message pump, window lifetime을 CraftEngine에 도입하게 된다.
- console을 핵심 표현 방식으로 유지한다는 현재 프로젝트 성격과 맞지 않는다.

향후 renderer 자체를 Win32/그래픽 window로 교체할 때는 자연스러운 후보지만 현재 MVP에는 사용하지 않는다.

### 후보 D: virtual terminal 입력 sequence 사용

표준 입력 stream에서 terminal escape sequence를 읽는 현대적인 방식이다. cross-platform과 원격 terminal 호환성에는 가장 유리하다.

장점:

- Windows Terminal을 포함한 terminal 중심 구조와 방향이 맞다.
- 장기적으로 다른 OS나 SSH/pseudoterminal로 확장하기 좋다.

단점:

- 일반적인 terminal key sequence는 key press 표현 위주이며 게임에 필요한 일관된 key-up/held 상태를 보장하기 어렵다.
- 방향키 escape sequence parsing, terminal별 차이, input mode 복원까지 별도 계층이 필요하다.
- 현재 Windows 전용 게임과 `GetKey` 상태 API를 위해 도입하기에는 복잡도가 높다.

출력 renderer를 virtual terminal 기반으로 옮기거나 cross-platform 요구가 생길 때 다시 검토한다.

### 후보 E: 테스트용 입력 분리

실제 입력 시스템을 바꾸지 않고 다음 방식으로 다중 client 검증만 분리한다.

- 실제 Z1 하나와 PowerShell dummy client 사용
- client별로 다른 scripted input 사용
- 한쪽 Z1 build에서 송신을 끄는 임시 실험
- 추후 command-line이나 config로 client별 key binding 지정

구현 위험이 가장 낮고 현재 replicated Actor 검증에는 충분하다. 다만 사용자 입력 문제의 제품 해법은 아니므로 임시 코드나 별도 build flag를 상시 유지하지 않는다.

## 권장 진행 순서

1. 현재 `NetworkPlayer`/`MyPlayer`와 snapshot 반영을 먼저 완료한다. 다중 client 검증은 실제 Z1 하나와 dummy client로 수행한다.
2. 입력 polling 책임을 `MyPlayer` 하나로 옮기고 `OverworldLevel::SendNetworkInput`을 제거해 Actor 책임을 먼저 정리한다.
3. 별도 CraftEngine 입력 개선 작업에서 `KEY_EVENT_RECORD` prototype을 만든다. 외부 `Input` API는 바꾸지 않는다.
4. Windows Terminal의 별도 window/tab/pane과 고전 console host에서 focus된 client만 입력을 받는지 확인한다.
5. Z1, ShootingGame, SokobanGame의 이동·key-down·key-up과 빠른 tap을 회귀 검증한다.
6. console input이 redirect된 실행, focus 전환 중 key release, key auto-repeat, `CTRL+C`, Quick Edit와 종료 시 console mode 복원을 확인한다.
7. 위 검증에서 Windows Terminal host가 low-level input record를 기대대로 격리하지 못할 경우 foreground 조건을 덧붙이지 말고, GUI input window 또는 terminal input library 도입 여부를 다시 결정한다.

## 완료 기준
 
 - 같은 PC에서 Z1 두 개를 실행하고 한 window/tab에 입력했을 때 해당 Session의 `C2S_Input`만 갱신된다.
 - focus를 바꾸거나 이동키를 놓았을 때 이전 Session에 `MoveDirection::None`이 전달되어 서버 이동이 멈춘다.
 - 원격 `NetworkPlayer`는 입력을 읽지 않고 snapshot으로만 움직인다.
 - key auto-repeat이 `GetKeyDown`을 반복 발생시키지 않는다.
 - 짧은 press/release가 같은 frame에 들어와도 `GetKeyDown`과 `GetKeyUp` edge를 잃지 않는다.
 - Z1, ShootingGame, SokobanGame의 기존 키 입력 동작에 회귀가 없다.
 - stdin이 console이 아닌 환경의 동작이 명시적이며 전역 polling으로 조용히 fallback하지 않는다.

## 구현 결과 (2026-09-04 적용 완료)

후보 A(`KEY_EVENT_RECORD` 및 `FOCUS_EVENT` 기반 콘솔 입력 버퍼 소비)가 `CraftEngine::Input`에 적용되었다.

- **버퍼 이벤트 소비**: `GetNumberOfConsoleInputEvents`로 논블로킹 확인 후 `ReadConsoleInputW`로 대기 중인 레코드를 일괄 drain하여 처리
- **키 상태 세분화**: `KeyState`를 `held`, `pressed`, `released`로 관리하여 키 반복(`bKeyDown=TRUE` 연속 수신) 시 `GetKeyDown` 중복 방지 및 1프레임 내 빠른 탭(Press/Release) 엣지 보존
- **포커스 이탈 대응**: `FOCUS_EVENT` 수신 시 `!bSetFocus`이면 눌려 있던 모든 키를 `released` 처리하여 창 전환 시 조작 멈춤(`None`) 보장
- **공개 API 유지**: 기존 `GetKey`, `GetKeyDown`, `GetKeyUp`의 시그니처와 의미를 유지하여 기존 게임 콘텐츠 호환성 확보
- **검증 완료**: 동일 PC에서 실제 Z1 클라이언트 2개를 실행하고 각 콘솔 창 포커스에 따라 독립적으로 입력(`C2S_Input`)이 전송되어 각 Player가 분리 조작됨을 확인

