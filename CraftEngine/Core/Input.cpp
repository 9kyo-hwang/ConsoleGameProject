#include "pch.h"
#include "Input.h"

namespace Craft
{
    Input* Input::_instance = nullptr;

    Input& Input::Get()
    {
        // TODO: 여기에 return 문을 삽입합니다.
        assert(_instance != nullptr && "Input 클래스 인스턴스가 존재하지 않음");
        return *_instance;
    }

    Input::Input()
    {
        assert(_instance == nullptr);
        _instance = this;
    }

    bool Input::GetKeyDown(int32 keyCode) const
    {
        return !_keyStates[keyCode].wasKeyDown && _keyStates[keyCode].isKeyDown;
    }

    bool Input::GetKeyUp(int32 keyCode) const
    {
        return _keyStates[keyCode].wasKeyDown && !_keyStates[keyCode].isKeyDown;
    }

    bool Input::GetKey(int32 keyCode) const
    {
        return _keyStates[keyCode].isKeyDown;
    }

    void Input::ProcessInput()
    {
        for (int32 key = 0; key < KeyCount; ++key)
        {
            _keyStates[key].isKeyDown = (GetAsyncKeyState(key) & 0x8000) != 0;
        }
    }

    void Input::SaveKeyStates()
    {
        for (KeyState& state : _keyStates)
        {
            state.wasKeyDown = state.isKeyDown;
        }
    }
}