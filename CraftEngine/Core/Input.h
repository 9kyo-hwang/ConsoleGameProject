#pragma once

#include <array>
#include <Core/Core.h>

namespace Craft
{
    class CRAFT_API Input
    {
    private:
        friend class Engine;

        struct KeyState
        {
            bool pressed = false;   // 현재 프레임 기준 눌렸는지
            bool held = false;      // 눌린 상태인지
            bool released = false;  // 현재 프레임 기준 뗐는지
        };

    public:
        static Input& Get();

    public:
        Input();
        ~Input() = default;

        bool GetKeyDown(int keyCode) const;   // 안눌렸다가 눌림
        bool GetKeyUp(int keyCode) const;     // 눌렸다가 떼짐
        bool GetKey(int keyCode) const;       // 현재 눌렸는지

    private:
        void ReadConsoleInputEvents();  // 현재 프레임에 키가 입력됐는지(only system)
        void HandleKeyEvent(const KEY_EVENT_RECORD& event);
        void HandleFocusEvent(const FOCUS_EVENT_RECORD& event);

    private:
        static Input* _instance;

        inline static constexpr unsigned short KeyCount = 256;
        std::array<KeyState, KeyCount> _keyStates{};

        HANDLE _handle = nullptr;
    };
}
