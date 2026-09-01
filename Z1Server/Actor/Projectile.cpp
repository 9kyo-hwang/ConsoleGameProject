#include "pch.h"
#include "Projectile.h"

using namespace Z1::Protocol;

Projectile::Projectile(std::uint32_t id, ProjectileKind kind, std::uint32_t ownerId, ServerRoomCoordinate home, Vector2Int spawnPosition, MoveDirection direction, std::int32_t damage, std::uint32_t lifetimeTicks)
    : _id(id), _kind(kind), _ownerId(ownerId), _home(home), _position(spawnPosition), _direction(direction), _damage(damage), _remainTicks(lifetimeTicks)
{
}

SnapshotProjectileState Projectile::BuildSnapshot() const
{
    SnapshotProjectileState snapshot;
    snapshot.id = _id;
    snapshot.kind = _kind;
    snapshot.x = _position.x;
    snapshot.y = _position.y;
    snapshot.direction = _direction;

    return snapshot;
}
