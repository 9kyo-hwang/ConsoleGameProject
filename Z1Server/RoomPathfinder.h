#pragma once
#include <Types.h>
#include <cmath>
#include <array>

using TileCoordinate = Vector2Int;          // Room 내부 논리 타일 좌표(7, 2)

struct RoomNavigationGrid
{
    static constexpr std::int32_t Width = 16;
    static constexpr std::int32_t Height = 11;

    bool IsWalkable(TileCoordinate tile) const noexcept
    {
        if (tile.x < 0 || tile.x >= Width || tile.y < 0 || tile.y >= Height)
        {
            return false;
        }

        return _walkable[Index(tile)];
    }

    void SetWalkable(TileCoordinate tile, bool isWalkable) noexcept
    {
        _walkable[Index(tile)] = isWalkable;
    }

private:
    static constexpr std::size_t Index(TileCoordinate tile) noexcept
    {
        return (std::size_t)(tile.y * Width + tile.x);
    }

    std::array<bool, Width* Height> _walkable{};
};

// 순수 A* 헬퍼 역할
class RoomPathfinder
{
public:
    std::vector<TileCoordinate> FindPath(const RoomNavigationGrid& grid, TileCoordinate start, TileCoordinate goal) const;

private:
    static std::int32_t ToIndex(TileCoordinate tile) noexcept;
    static TileCoordinate ToTile(std::int32_t index) noexcept;
};

