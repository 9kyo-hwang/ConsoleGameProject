#pragma once
#include <filesystem>
#include <Math/Vector2.h>
#include <array>
#include <Level/Room.h>

using TileId = std::uint8_t;    // 00, 01, ..., 9c, 9d. 지형 구분 Id
constexpr TileId InvalidTileId = 0xff;

class OverworldMap
{
    using FilePath = std::filesystem::path;

public:
    // Map을 구성하는 Room 개수
    static constexpr int RoomColumns = 16;
    static constexpr int RoomRows = 8;

    // Map을 구성하는 타일 개수
    static constexpr int Width = RoomTileWidth * RoomColumns;
    static constexpr int Height = RoomTileHeight * RoomRows;

public:
    bool Load(
        const FilePath& tileMapPath,
        const FilePath& blockingMapPath,
        std::string& errorMessage
    );

    TileId GetTileId(int x, int y) const;
    
    bool CanOccupyWorldRect(
        const Craft::Vector2& boxColliderPosition,
        const Craft::Vector2& boxColliderSize,
        const Craft::Vector2& tileSize
    ) const;

private:
    struct Cell
    {
        TileId id = InvalidTileId;
        bool walkable = false;
    };

    using Row = std::array<Cell, Width>;
    using Grid = std::array<Row, Height>;

    bool ParseTileMap(const FilePath& path, Grid& output, std::string& errorMessage);
    bool ParseBlockingMap(const FilePath& path, Grid& output, std::string& errorMessage);

    bool OutOfBound(int x, int y) const;
    bool IsWalkable(int x, int y) const;

private:
    bool _loaded = false;
    Grid _cells{};
};

