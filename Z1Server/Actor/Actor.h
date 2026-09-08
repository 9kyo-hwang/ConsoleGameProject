#pragma once
#include <Z1Shared/Protocol.h>
#include <Types.h>
#include <atomic>

class Actor
{
public:
    explicit Actor(Z1::Protocol::ActorKind kind);

    Actor(const Actor&) = delete;
    Actor& operator=(const Actor&) = delete;
    Actor(Actor&&) = default;
    Actor& operator=(Actor&&) = default;

    const Z1::Protocol::ActorInfo& GetInfo() const noexcept { return info; }
    std::uint32_t GetId() const noexcept { return info.id; }

    Z1::Protocol::MoveDirection GetDirection() const noexcept { return info.direction; }
    void SetDirection(Z1::Protocol::MoveDirection direction) { info.direction = direction; }

    Vector2Int GetPosition() const noexcept { return { info.x, info.y }; }
    void SetPosition(std::int32_t x, std::int32_t y) { info.x = x; info.y = y; }

protected:
    Z1::Protocol::ActorInfo info;

private:
    inline static std::atomic_uint32_t sIdGenerator = 1;
};

