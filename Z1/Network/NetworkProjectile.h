#pragma once
#include <Actor/Actor.h>
#include <Z1Shared/Protocol.h>

namespace Craft
{
    class Sprite;
    class SpriteRendererComponent;
}

class NetworkProjectile : public Craft::Actor
{
    TYPE_DECLARATIONS(NetworkProjectile, Craft::Actor)

public:
    NetworkProjectile(Craft::Vector2 position, std::uint32_t id, Z1::Protocol::ActorKind kind, Z1::Protocol::MoveDirection direction);

    void ApplySnapshot(const Z1::Protocol::ActorInfo& state);

private:
    std::shared_ptr<Craft::SpriteRendererComponent> _renderer;

    std::uint32_t _id = 0;
    Z1::Protocol::ActorKind _kind;
    Z1::Protocol::MoveDirection _direction;
};

