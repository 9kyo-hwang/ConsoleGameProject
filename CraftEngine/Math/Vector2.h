#pragma once

#include <Core/Core.h>
#include <Windows.h>

namespace Craft
{
    class CRAFT_API Vector2
    {
    public:
        Vector2(int x = 0, int y = 0);
        ~Vector2();

    public:
        operator COORD() const noexcept;

        Vector2 operator+(const Vector2& other) const;
        Vector2 operator-(const Vector2& other) const;
        Vector2 operator*(const Vector2& other) const;
        Vector2 operator*(int value) const;
        Vector2 operator/(const Vector2& other) const;

        Vector2& operator=(const Vector2& other);

        bool operator==(const Vector2& other) const;
        bool operator!=(const Vector2& other) const;

    public:
        static Vector2 Zero;
        static Vector2 One;
        static Vector2 Right;
        static Vector2 Up;

    public:
        // 좌상단 기준 (0, 0), 오른쪽/아래로 갈 수록 +
        int x = 0;
        int y = 0;
    };
}