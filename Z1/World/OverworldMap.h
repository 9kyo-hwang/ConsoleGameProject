#pragma once
#include <filesystem>
#include <Math/Vector2.h>
#include <array>
#include <Level/Room.h>
#include <Math/Box2D.h>

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
    
    bool CanPlaceBox(const Craft::Box2D& box) const;

private:
    struct Tile
    {
        TileId id = InvalidTileId;
        bool walkable = false;
    };

    using TileRow = std::array<Tile, Width>;
    using TileMap = std::array<TileRow, Height>;

    bool ParseTileMap(const FilePath& path, TileMap& output, std::string& errorMessage);
    bool ParseBlockingMap(const FilePath& path, TileMap& output, std::string& errorMessage);

    bool OutOfBound(int x, int y) const;
    bool IsWalkable(int x, int y) const;

private:
    bool _loaded = false;
    TileMap _tiles{};
};

