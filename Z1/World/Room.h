#pragma once
#include <array>
#include <World/Tile.h>

struct RoomDefinition
{
    static constexpr int Width = 16;
    static constexpr int Height = 11;
    static constexpr int TileCount = Width * Height;

    // 전체 Map에서 Room이 위치하는 인덱스. [0, 16) [0, 8)
    int roomX = -1;
    int roomY = -1;

    std::array<TileId, TileCount> tileIds{};
    std::array<uint8_t, TileCount> walkable{};

    bool hasHalfHeightBottomRow = true;

    inline static constexpr int Index(int x, int y) { return y * Width + x; }
    bool OutOfBound(int x, int y) const;
    TileId GetTileId(int x, int y) const;
    bool IsWalkable(int x, int y) const;

    bool CanOccupyWorldRect(const Craft::Vector2& localWorldPosition, const Craft::Vector2& worldSize, const Craft::Vector2& tileWorldSize) const;
    bool CanOccupyTiles(int minTileX, int minTileY, int maxTileX, int maxTileY) const;
};