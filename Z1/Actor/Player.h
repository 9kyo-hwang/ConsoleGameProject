#pragma once
#include <Actor/Actor.h>

namespace Craft
{
    class BoxComponent;
    class SpriteRendererComponent;
    class Sprite;
}

enum Facing
{
    Up = 0,
    Right,
    Down,
    Left
};

class Player : public Craft::Actor
{
    TYPE_DECLARATIONS(Player, Craft::Actor)

public:
    Player(Craft::Vector2 position, Craft::Vector2 renderScale);

    void MoveBy(const Craft::Vector2& delta);

    inline Facing GetFacing() const { return _facing; }
    void SetFacing(Facing facing) { _facing = facing; }

private:
    std::shared_ptr<const Craft::Sprite> CreateSprite();

private:
    std::shared_ptr<Craft::BoxComponent> _box;
    std::shared_ptr<Craft::SpriteRendererComponent> _renderer;

    Facing _facing = Facing::Down;
};
