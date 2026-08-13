#pragma once

#include <Math/Vector2.h>
#include <Level/Room.h>
#include <vector>

class OverworldMap;

struct EnemySpawnData
{
    Craft::Vector2 worldPosition;
    int maxHp = 2;
};

// Not Actor
class EnemySpawner
{
public:
    std::vector<EnemySpawnData> BuildSpawnPlan(
        RoomCoordinate room,
        const OverworldMap& map,
        const Craft::Vector2& playerPosition,
        const Craft::Vector2& mapTileSize,
        uint32_t worldSeed,
        const Craft::Vector2& roomOrigin) const;

private:
    uint32_t MakeRoomSeed(RoomCoordinate room, uint32_t worldSeed) const;
};

