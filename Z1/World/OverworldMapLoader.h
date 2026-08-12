#pragma once
#include <filesystem>
#include <World/Room.h>

class OverworldMapData
{
public:
    static constexpr int RoomColumns = 16;
    static constexpr int RoomRows = 8;
    static constexpr int Width = RoomColumns * RoomData::Width;   // 256
    static constexpr int Height = RoomRows * RoomData::Height; // 88
    
public:
    inline static constexpr int Index(int x, int y) { return y * Width + x; }
    bool OutOfBound(int x, int y) const;

    TileId GetTileId(int x, int y) const;
    void SetTileId(int x, int y, TileId id);
    
    bool IsWalkable(int x, int y) const;
    void SetWalkable(int x, int y, bool walkable);

    bool CanOccupyWorldRect(
        const Craft::Vector2& boxColliderPosition, 
        const Craft::Vector2& boxColliderSize, 
        const Craft::Vector2& tileSize
    ) const;

private:
    std::array<TileId, Width * Height> _tileIds{};
    std::array<uint8_t, Width * Height> _walkables{};
};

using FilePath = std::filesystem::path;
class OverworldMapLoader
{
public:
    // Path: ../Content/Z1/Maps/Overworld/TileMap.txt, BlockingMap.txt
    bool Load(const FilePath& tileMapPath, const FilePath& blockingMapPath, std::string& errorMessage);
    bool IsLoaded() const { return _loaded; }

    bool CanOccupyWorldRect(
        const Craft::Vector2& boxColliderPosition, 
        const Craft::Vector2& boxColliderSize, 
        const Craft::Vector2& tileSize
    ) const;

    std::optional<RoomData> ExtractRoom(RoomCoordinate room, std::string& errorMessage) const;

private:
    OverworldMapData _data;
    bool _loaded = false;
};

