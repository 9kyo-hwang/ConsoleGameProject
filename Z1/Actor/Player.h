#pragma once
#include <Actor/Actor.h>

namespace Craft
{
    class BoxComponent;
    class SpriteRendererComponent;
    class Sprite;
}

class Player : public Craft::Actor
{
    TYPE_DECLARATIONS(Player, Craft::Actor)

public:
    Player(Craft::Vector2 position, Craft::Vector2 renderScale);

    void MoveBy(const Craft::Vector2& delta);

private:
    std::shared_ptr<const Craft::Sprite> CreateSprite();

private:
    std::shared_ptr<Craft::BoxComponent> _box;
    std::shared_ptr<Craft::SpriteRendererComponent> _renderer;
};
