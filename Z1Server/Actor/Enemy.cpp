#include "pch.h"
#include "Enemy.h"

using namespace Z1::Protocol;

Enemy::Enemy(std::uint32_t id, EnemyKind kind, ServerRoomCoordinate home, Vector2Int position)
    : _id(id), _kind(kind), _home(home), _spawnPosition(position), _position(position)
{
}

SnapshotEnemyState Enemy::BuildSnapshot() const
{
    SnapshotEnemyState snapshot;
    snapshot.id = _id;
    snapshot.kind = _kind;
    snapshot.x = _position.x;
    snapshot.y = _position.y;
    snapshot.hp = _hp;
    snapshot.facing = _facing;
    snapshot.flags = 0; // 현재는 공격이 없으므로

    return snapshot;
}

void Enemy::MoveTo(Vector2Int position, MoveDirection facing)
{
    // 값만 갱신해도 Snapshot이 Broadcast되면서 반영됨
    _position = position;
    _facing = facing;
}

Vector2Int Enemy::GetProjectileSpawnPosition()
{
    return Vector2Int
    {
        _position.x + (BoxWidth - 1) / 2,
        _position.y + (BoxHeight - 1) /2,
    };
}
