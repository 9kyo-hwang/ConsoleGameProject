#pragma once

struct RoomCoordinate
{
    int x = 0;
    int y = 0;

    auto operator<=>(const RoomCoordinate&) const = default;
};

// Room 하나가 차지하는 타일 개수
static constexpr int RoomTileWidth = 16;
static constexpr int RoomTileHeight = 11;