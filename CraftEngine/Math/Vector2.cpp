#include "pch.h"
#include "Vector2.h"

namespace Craft
{
    Vector2 Vector2::Zero(0, 0);
    Vector2 Vector2::One(1, 1);
    Vector2 Vector2::Right(1, 0);
    Vector2 Vector2::Up(0, -1); // 내려가는 게 +라 윗방향은 -

    Vector2::Vector2(int32 x, int32 y)
        : x(x), y(y)
    {
    }

    Vector2::~Vector2()
    {
    }

    Vector2::operator COORD() const noexcept
    {
        return COORD{ (SHORT)x, (SHORT)y };
    }

    Vector2 Vector2::operator+(const Vector2& other) const
    {
        return Vector2(x + other.x, y + other.y);
    }

    Vector2 Vector2::operator-(const Vector2& other) const
    {
        return Vector2(x - other.x, y - other.y);
    }

    Vector2 Vector2::operator*(const Vector2& other) const
    {
        return Vector2(x * other.x, y * other.y);
    }

    Vector2 Vector2::operator*(int32 value) const
    {
        return Vector2(x * value, y * value);
    }

    Vector2 Vector2::operator/(const Vector2& other) const
    {
        assert(other.x != 0 && other.y != 0);
        return Vector2(x / other.x, y / other.y);
    }

    Vector2& Vector2::operator=(const Vector2& other)
    {
        x = other.x;
        y = other.y;

        return *this;
    }

    bool Vector2::operator!=(const Vector2& other) const
    {
        return !(*this == other);
    }
}