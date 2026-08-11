#pragma once
#include <filesystem>
#include <World/Room.h>

struct OverworldMapData
{
    static constexpr int RoomColumns = 16;
    static constexpr int RoomRows = 8;
    static constexpr int Width = RoomColumns * RoomDefinition::Width;   // 256
    static constexpr int Height = RoomRows * RoomDefinition::Height; // 88

    std::array<TileId, Width* Height> tileIds{};
    std::array<uint8_t, Width* Height> walkable{};
    
    inline static constexpr int Index(int x, int y) { return y * Width + x; }
    bool OutOfBound(int x, int y) const;
    TileId GetTileId(int x, int y) const;
    bool IsWalkable(int x, int y) const;
};

using FilePath = std::filesystem::path;
class OverworldMapLoader
{
public:
    // Path: ../Content/Z1/Maps/Overworld/TileMap.txt, BlockingMap.txt
    bool Load(const FilePath& tileMapPath, const FilePath& blockingMapPath, std::string& errorMessage);
    bool IsLoaded() const { return _loaded; }

    std::optional<RoomDefinition> ExtractRoom(int roomX, int roomY, std::string& errorMessage) const;

private:
    OverworldMapData _data;
    bool _loaded = false;
};

