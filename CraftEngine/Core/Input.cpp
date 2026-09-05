#include "pch.h"
#include "Input.h"

namespace Craft
{
    Input* Input::_instance = nullptr;

    Input& Input::Get()
    {
        assert(_instance != nullptr && "Input 클래스 인스턴스가 존재하지 않음");
        return *_instance;
    }

    Input::Input()
    {
        assert(_instance == nullptr);
        _instance = this;

        _handle = ::GetStdHandle(STD_INPUT_HANDLE);
        assert(_handle != INVALID_HANDLE_VALUE && "표준 입력 핸들을 가져올 수 없음");
    }

    bool Input::GetKeyDown(int32 keyCode) const
    {
        if (keyCode < 0 || keyCode >= KeyCount) return false;
        return _keyStates[keyCode].pressed;
    }

    bool Input::GetKeyUp(int32 keyCode) const
    {
        if (keyCode < 0 || keyCode >= KeyCount) return false;
        return _keyStates[keyCode].released;
    }

    bool Input::GetKey(int32 keyCode) const
    {
        if (keyCode < 0 || keyCode >= KeyCount) return false;
        return _keyStates[keyCode].held;
    }

    void Input::ReadConsoleInputEvents()
    {
        if (_handle == INVALID_HANDLE_VALUE)
        {
            return;
        }

        // 매 프레임 초기화
        for (KeyState& state : _keyStates)
        {
            state.pressed = false;
            state.released = false;
        }

        DWORD numEvents = 0;
        if (!::GetNumberOfConsoleInputEvents(_handle, &numEvents) || numEvents == 0)
        {
            return;
        }

        constexpr DWORD Length = 128;
        INPUT_RECORD buffer[Length]{};
        DWORD eventsRead = 0;
        while (numEvents > 0)
        {
            DWORD remain = std::min<DWORD>(numEvents, Length);
            if (!::ReadConsoleInputW(_handle, buffer, remain, &eventsRead) || eventsRead == 0)
            {
                break;
            }

            numEvents -= eventsRead;
            for (DWORD i = 0; i < eventsRead; ++i)
            {
                const INPUT_RECORD& record = buffer[i];
                switch (record.EventType)
                {
                case KEY_EVENT: HandleKeyEvent(record.Event.KeyEvent); break;
                case FOCUS_EVENT: HandleFocusEvent(record.Event.FocusEvent); break;
                }
            }
        }
    }

    void Input::HandleKeyEvent(const KEY_EVENT_RECORD& event)
    {
        WORD keyCode = event.wVirtualKeyCode;
        if (keyCode >= KeyCount)
        {
            return;
        }

        KeyState& state = _keyStates[keyCode];
        if (event.bKeyDown)
        {
            // 이미 눌려져있는 경우 repeat 이벤트 무시
            if (!state.held)
            {
                state.held = true;
                state.pressed = true;
            }
        }
        else  // 키 입력을 중지한 경우
        {
            // 현재 프레임에 눌려져있었다면
            if (state.held)
            {
                state.held = false;
                state.released = true;
            }
        }
    }

    void Input::HandleFocusEvent(const FOCUS_EVENT_RECORD& event)
    {
        // 현재 창에서 다른 창으로 포커싱을 옮겼을 때
        if (!event.bSetFocus)
        {
            for (KeyState& state : _keyStates)
            {
                // 현재 눌려지고 있는 키들 전부 뗀 상태로 전환
                if (state.held)
                {
                    state.held = false;
                    state.released = true;
                }
            }
        }
    }
}
