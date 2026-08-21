#include "pch.h"
#include "EnemySpawner.h"
#include <World/OverworldMap.h>
#include <Math/MathUtility.h>
#include <set>
#include <Level/Room.h>

using namespace Craft;

namespace 
{
    constexpr int EnemyCounts = 6;
    constexpr int MaxSpawnAttempts = 30;
}

std::vector<EnemySpawnData> EnemySpawner::BuildSpawnPlan(RoomCoordinate room, const OverworldMap& map, const Vector2& playerPosition, const Vector2& mapTileSize, uint32_t worldSeed, const Vector2& roomOrigin) const
{
    FMath::SetRandomSeed(MakeRoomSeed(room, worldSeed));

    std::vector<EnemySpawnData> data;
    std::set<Vector2> selected;
    for (int attempt = 0; attempt < MaxSpawnAttempts && (int)selected.size() < EnemyCounts; ++attempt)
    {
        // 논리 타일 좌표를
        const int x = FMath::RandRange(1, RoomTileWidth - 2);
        const int y = FMath::RandRange(1, RoomTileHeight - 2);

        // 월드 셀 좌표로
        const Vector2 worldPosition = roomOrigin + Vector2(x, y) * mapTileSize;

        // 적이 배치될 수 없는 위치면 pass
        if (!map.CanOccupyWorldRect(worldPosition, Vector2(10, 5), mapTileSize))
        {
            continue;
        }

        // 너무 가까워도 pass
        const Vector2 distance = worldPosition - playerPosition;
        if (std::abs(distance.x) < mapTileSize.x << 1 &&
            std::abs(distance.y) < mapTileSize.y << 1)
        {
            continue;
        }

        // 이미 스폰된 위치면 pass
        if (selected.contains(worldPosition))
        {
            continue;
        }

        selected.emplace(worldPosition);

        EnemyKind kind = EnemyKind::Octorok;
        switch (FMath::RandRange(0, 2))
        {
        case 1: kind = EnemyKind::Moblin; break;
        case 2: kind = EnemyKind::Tektite; break;
        }

        data.push_back(EnemySpawnData
            {
                .kind = kind,
                .variant = EnemyVariant::Red,
                .worldPosition = worldPosition
            });
    }

    return data;
}

uint32_t EnemySpawner::MakeRoomSeed(RoomCoordinate room, uint32_t worldSeed) const
{
    return worldSeed
        ^ (uint32_t)room.x * 73856093u
        ^ (uint32_t)room.y * 19349663u;
}
