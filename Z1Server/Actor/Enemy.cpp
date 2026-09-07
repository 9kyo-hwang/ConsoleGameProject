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

std::int32_t Enemy::TakeDamage(std::int32_t damage, std::uint32_t serverTick)
{
    if (damage <= 0 || IsDead()) return 0;

    std::int32_t actualDamage = std::min<std::int32_t>(_hp, damage);
    _hp -= actualDamage;

    if (_hp == 0)
    {
        _dead = true;
        _deadTick = serverTick;
    }

    return actualDamage;
}

Vector2Int Enemy::GetProjectileSpawnPosition()
{
    return Vector2Int
    {
        _position.x + (BoxWidth - 1) / 2,
        _position.y + (BoxHeight - 1) /2,
    };
}

void Enemy::Respawn()
{
    _dead = false;
    _deadTick = 0;
    _hp = 1;
    _position = _spawnPosition;
    _facing = MoveDirection::Up;
    ResetAttackCooldown();
}
