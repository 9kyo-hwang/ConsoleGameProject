#pragma once

#include <Z1Shared/Protocol.h>
#include <Types.h>

// 값 객체
class Projectile
{
public:
    Projectile(std::uint32_t id, Z1::Protocol::ProjectileKind kind, std::uint32_t ownerId,
        ServerRoomCoordinate home, Vector2Int spawnPosition, Z1::Protocol::MoveDirection direction,
        std::int32_t damage, std::uint32_t lifetimeTicks);

    Z1::Protocol::SnapshotProjectileState BuildSnapshot() const;

    inline std::uint32_t GetId() const noexcept { return _id; }
    inline ServerRoomCoordinate GetHomeRoom() const noexcept { return _home; }
    inline Vector2Int GetPosition() const noexcept { return _position; }
    inline Z1::Protocol::MoveDirection GetDirection() const noexcept { return _direction; }

    inline std::int32_t GetDamage() const noexcept { return _damage; }
    inline std::uint32_t GetOwnerId() const noexcept { return _ownerId; }

    void MoveTo(Vector2Int position) noexcept { _position = position; }

    inline void ElapseTick() noexcept { if (_remainTicks > 0) --_remainTicks; }
    inline bool IsExpired() const noexcept { return _remainTicks == 0; }

private:
    std::uint32_t _id = 0;
    Z1::Protocol::ProjectileKind _kind;
    std::uint32_t _ownerId = 0;

    ServerRoomCoordinate _home;
    Vector2Int _position;
    Z1::Protocol::MoveDirection _direction;

    std::int32_t _damage = 1;
    std::uint32_t _remainTicks = 0;
};

