#include "pch.h"
#include "CollisionTestActor.h"
#include <Component/BoxComponent.h>
using namespace Craft;

CollisionTestActor::CollisionTestActor(Vector2 position, Vector2 size, Vector2 offset)
    : Super(position)
{
    _boxCollider = AddComponent<BoxComponent>(size, offset);
}

void CollisionTestActor::OnCollision(const std::shared_ptr<Actor>& other)
{
    ++_numCollision;
    Super::OnCollision(other);
}
