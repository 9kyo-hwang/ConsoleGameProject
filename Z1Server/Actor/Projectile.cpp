#include "pch.h"
#include "Projectile.h"

using namespace Z1::Protocol;

Projectile::Projectile(ActorKind kind, std::uint32_t ownerId, ServerRoomCoordinate home, Vector2Int spawnPosition, MoveDirection direction, std::int32_t damage, std::uint32_t lifetimeTicks)
    : Actor(kind), _ownerId(ownerId), _home(home), _damage(damage), _remainTicks(lifetimeTicks)
{
    SetPosition(spawnPosition.x, spawnPosition.y);
    info.direction = direction;
}

