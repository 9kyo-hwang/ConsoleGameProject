#pragma once

#include <Actor/Actor.h>
#include <Types.h>

// 값 객체
class Projectile : public Actor
{
public:
    Projectile(Z1::Protocol::ActorKind kind, std::uint32_t ownerId,
        ServerRoomCoordinate home, Vector2Int spawnPosition, Z1::Protocol::MoveDirection direction,
        std::int32_t damage, std::uint32_t lifetimeTicks);

    inline ServerRoomCoordinate GetHomeRoom() const noexcept { return _home; }
    inline std::int32_t GetDamage() const noexcept { return _damage; }
    inline std::uint32_t GetOwnerId() const noexcept { return _ownerId; }

    inline void ElapseTick() noexcept { if (_remainTicks > 0) --_remainTicks; }
    inline bool IsExpired() const noexcept { return _remainTicks == 0; }

private:
    std::uint32_t _ownerId = 0;

    ServerRoomCoordinate _home;

    std::int32_t _damage = 1;
    std::uint32_t _remainTicks = 0;
};

