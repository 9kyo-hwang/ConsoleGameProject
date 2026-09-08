#include "pch.h"
#include "Enemy.h"

using namespace Z1::Protocol;

Enemy::Enemy(Z1::Protocol::ActorKind kind, ServerRoomCoordinate home, Vector2Int spawnPosition)
    : Actor(kind), _home(home), _spawnPosition(spawnPosition)
{
    // TODO: Kind가 Enemy 계열인지 검증
    info.hp = 1;
    SetPosition(spawnPosition.x, spawnPosition.y);
}

void Enemy::SetPosition(std::int32_t x, std::int32_t y)
{
    Actor::SetPosition(x, y);
    
    if (_nextWaypoint == Vector2Int{ x, y })
    {
        ClearNextWaypoint();
    }
}

std::int32_t Enemy::TakeDamage(std::int32_t damage, std::uint32_t serverTick)
{
    if (damage <= 0 || IsDead()) return 0;

    std::int32_t actualDamage = std::min<std::int32_t>(info.hp, damage);
    info.hp -= actualDamage;

    if (info.hp == 0)
    {
        _deadTick = serverTick;
    }

    return actualDamage;
}

Vector2Int Enemy::GetProjectileSpawnPosition()
{
    return Vector2Int
    {
        info.x + (BoxWidth - 1) / 2,
        info.y + (BoxHeight - 1) / 2,
    };
}

void Enemy::Respawn()
{
    _nextWaypoint.reset();

    _deadTick = 0;
    info.hp = 1;
    SetPosition(_spawnPosition.x, _spawnPosition.y);
    SetDirection(MoveDirection::Up);
    ResetAttackCooldown();
}
