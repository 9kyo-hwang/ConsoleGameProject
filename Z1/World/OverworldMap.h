#pragma once
#include <filesystem>
#include <Math/Vector2.h>
#include <array>
#include <Level/Room.h>
#include <Math/Box2D.h>
#include <Tilemaps/Tilemap.h>
#include <unordered_map>

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
    OverworldMap();

    bool Load(const FilePath& tileMapPath, const FilePath& blockingMapPath, std::string& errorMessage);

    TileId GetTileId(int x, int y) const;
    
    std::shared_ptr<const Craft::Sprite> BuildRoomSprite(Craft::Vector2 roomOrigin, Craft::Vector2 roomSize) const;
    bool CanPlaceBox(const Craft::Box2D& box) const;

private:
    void InitializeTileSprites();

    bool ParseTileMap(const FilePath& path, std::vector<TileId>& tileIds, std::string& errorMessage);
    bool ParseBlockingMap(const FilePath& path, std::vector<bool>& blocked, std::string& errorMessage);

private:
    bool _loaded = false;
    Craft::Tilemap _tilemap;
    std::vector<TileId> _tileIds;
    std::unordered_map<TileId, std::shared_ptr<const Craft::Sprite>> _tileSprites;
};

