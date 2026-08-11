#pragma once
#include <Actor/Actor.h>

namespace Craft
{
    class BoxComponent;
}

class CollisionTestActor : public Craft::Actor
{
    TYPE_DECLARATIONS(CollisionTestActor, Craft::Actor)

public:
    CollisionTestActor(Craft::Vector2 position, Craft::Vector2 size = Craft::Vector2::One, Craft::Vector2 offset = Craft::Vector2::Zero);
    ~CollisionTestActor() override = default;

    void OnCollision(const std::shared_ptr<Craft::Actor>& other) override;

    inline int GetCollisionCount() const { return _numCollision; }

private:
    std::shared_ptr<Craft::BoxComponent> _boxCollider = nullptr;
    int _numCollision = 0;
};

