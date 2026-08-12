#pragma once
#include <array>
#include <World/Tile.h>

/*
* 이동/충돌 판정에 대한 책임을 Map으로 이관
*/
struct RoomData
{
    static constexpr int Width = 16;
    static constexpr int Height = 11;
    static constexpr int TileCount = Width * Height;

    std::array<TileId, TileCount> tileIds{};

    inline static constexpr int Index(int x, int y) { return y * Width + x; }
    bool OutOfBound(int x, int y) const;
    TileId GetTileId(int x, int y) const;
};

struct RoomCoordinate
{
    int x = 0;
    int y = 0;

    bool operator==(const RoomCoordinate&) const = default;
};