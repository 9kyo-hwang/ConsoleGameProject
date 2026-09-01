#include "pch.h"
#include "Enemy.h"

Enemy::Enemy(std::uint32_t id, Z1::Protocol::EnemyKind kind, ServerRoomCoordinate home, Vector2Int position)
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

void Enemy::MoveTo(Vector2Int position, Z1::Protocol::MoveDirection facing)
{
    // 값만 갱신해도 Snapshot이 Broadcast되면서 반영됨
    _position = position;
    _facing = facing;
}
