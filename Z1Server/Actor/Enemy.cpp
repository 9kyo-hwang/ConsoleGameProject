#include "pch.h"
#include "Enemy.h"

Enemy::Enemy(std::uint32_t id, Z1::Protocol::EnemyKind kind, ServerRoomCoordinate home, Vector2Int position)
{
}

SnapshotEnemyState Enemy::BuildSnapshot() const
{
    return SnapshotEnemyState();
}
