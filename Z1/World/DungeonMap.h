#pragma once
#include <filesystem>
#include <Math/Vector2.h>
#include <array>
#include <Level/Room.h>
#include <Math/Box2D.h>

class DungeonMap
{
    using FilePath = std::filesystem::path;

public:
    // Map을 구성하는 Room 개수
    static constexpr int RoomColumns = 5;
    static constexpr int RoomRows = 1;

    // Map을 구성하는 타일 개수
    static constexpr int Width = RoomTileWidth * RoomColumns;
    static constexpr int Height = RoomTileHeight * RoomRows;

    using TileRow = std::array<char, Width>;
    using TileMap = std::array<TileRow, Height>;

public:
    bool Load(
        const FilePath& path,
        std::string& errorMessage
    );

    bool CanPlaceBox(const Craft::Box2D& box) const;

    char GetTile(int x, int y) const;
    bool IsWalkable(int x, int y) const;

private:
    bool _loaded = false;
    TileMap _tiles{};
};

