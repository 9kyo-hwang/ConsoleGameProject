#pragma once
#include <Actor/Actor.h>
#include <Z1Shared/Protocol.h>

namespace Craft
{
    class Sprite;
    class SpriteRendererComponent;
}

class NetworkEnemy : public Craft::Actor
{
    TYPE_DECLARATIONS(NetworkEnemy, Craft::Actor)

public:
    NetworkEnemy(Craft::Vector2 position, std::uint32_t id, Z1::Protocol::EnemyKind kind);

    void ApplySnapshot(const Z1::Protocol::SnapshotEnemyState& state);

private:
    std::shared_ptr<const Craft::Sprite> CreateSprite();

private:
    std::shared_ptr<Craft::SpriteRendererComponent> _renderer;

    std::uint32_t _id = 0;
    Z1::Protocol::EnemyKind _kind;
    Z1::Protocol::MoveDirection _facing = Z1::Protocol::MoveDirection::Up;
    std::int32_t _hp = 0;
    std::uint8_t _flags = 0;
};
