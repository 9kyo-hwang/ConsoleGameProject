#pragma once

#include <cstdint>
#include <compare>

struct Vector2Int
{
    int x = 0;
    int y = 0;

    auto operator<=>(const Vector2Int&) const = default;
};

struct ServerRoomCoordinate
{
    std::int32_t x = 0;
    std::int32_t y = 0;

    auto operator<=>(const ServerRoomCoordinate&) const = default;
};
