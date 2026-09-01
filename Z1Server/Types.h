#pragma once

#include <cstdint>
#include <compare>

// 서버의 월드 맵 좌표(셀 기준. 1190, 395)
struct Vector2Int
{
    std::int32_t x = 0;
    std::int32_t y = 0;

    auto operator<=>(const Vector2Int&) const = default;

    Vector2Int operator+(const Vector2Int& other) const
    {
        return Vector2Int(x + other.x, y + other.y);
    }

    Vector2Int operator-(const Vector2Int& other) const
    {
        return Vector2Int(x - other.x, y - other.y);
    }

    std::int32_t LengthSquared() const noexcept
    {
        return x * x + y * y;
    }
};

using ServerRoomCoordinate = Vector2Int;    // 월드 Room 좌표(7, 2)