#pragma once
#include <filesystem>
#include <Math/Vector2.h>
#include <array>
#include <Level/Room.h>
#include <Math/Box2D.h>
#include <Tilemaps/Tilemap.h>
#include <unordered_map>

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

public:
    DungeonMap();

    bool Load(const FilePath& path, std::string& errorMessage);
    std::shared_ptr<const Craft::Sprite> BuildRoomSprite(Craft::Vector2 roomOrigin, Craft::Vector2 roomSize) const;
    bool CanPlaceBox(const Craft::Box2D& box) const;

    char GetTile(int x, int y) const;

private:
    void InitializeTileSprites();

private:
    bool _loaded = false;
    std::vector<char> _tiles;
    std::unordered_map<char, std::shared_ptr<const Craft::Sprite>> _tileSprites;
    Craft::Tilemap _tilemap;
};

