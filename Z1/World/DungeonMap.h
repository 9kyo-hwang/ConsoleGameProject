#pragma once
#include <filesystem>
#include <Math/Vector2.h>
#include <array>
#include <Level/Room.h>

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

    using Row = std::array<char, Width>;
    using Grid = std::array<Row, Height>;

public:
    bool Load(
        const FilePath& path,
        std::string& errorMessage
    );

    bool CanOccupyWorldRect(
        const Craft::Vector2& boxColliderPosition,
        const Craft::Vector2& boxColliderSize,
        const Craft::Vector2& tileSize
    ) const;

    char GetTile(int x, int y) const;
    bool IsWalkable(int x, int y) const;

private:
    bool _loaded = false;
    Grid _grid{};
};

