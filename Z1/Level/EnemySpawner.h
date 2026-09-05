#pragma once

#include <Math/Vector2.h>
#include <Level/Room.h>
#include <vector>
#include <Actor/EnemyTypes.h>

class OverworldMap;

struct EnemySpawnData
{
    EnemyKind kind = EnemyKind::Octorok;
    EnemyVariant variant = EnemyVariant::Red;

    Craft::Vector2 mapCellPosition;
};

// Not Actor
class EnemySpawner
{
public:
    std::vector<EnemySpawnData> BuildSpawnPlan(
        RoomCoordinate room,
        const OverworldMap& map,
        const Craft::Vector2& playerPosition,
        uint32_t worldSeed,
        const Craft::Vector2& roomOrigin) const;

private:
    uint32_t MakeRoomSeed(RoomCoordinate room, uint32_t worldSeed) const;
};

