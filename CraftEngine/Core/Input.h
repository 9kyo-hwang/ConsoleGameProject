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
            bool isKeyDown = false;     // 현재 프레임에 키가 눌렸는지
            bool wasKeyDown = false;    // 이전 프레임에 키가 눌렸었는지
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
        void ProcessInput();  // 현재 프레임에 키가 입력됐는지(only system)
        void SaveKeyStates(); // 현재 프레임 입력 상태 기록

    private:
        inline static constexpr unsigned short KeyCount = 256;
        std::array<KeyState, KeyCount> _keyStates{};

        static Input* _instance;
    };
}
