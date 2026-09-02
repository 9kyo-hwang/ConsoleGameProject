#include "pch.h"
#include "NetworkSwordEffect.h"
#include <Component/SpriteRendererComponent.h>
#include <Actor/SwordSprite.h>

using namespace Craft;
using namespace Z1::Protocol;

namespace
{
    Vector2 ToVectorDirection(MoveDirection direction)
    {
        switch (direction)
        {
        case MoveDirection::Up: return Vector2::Up;
        case MoveDirection::Down: return Vector2::Up * -1;
        case MoveDirection::Left: return Vector2::Right * -1;
        case MoveDirection::Right: return Vector2::Right;
        }
    };

    Vector2 GetSwordOffset(MoveDirection direction)
    {
        switch (direction)
        {
        case MoveDirection::Up: return Vector2(1, -5);
        case MoveDirection::Down: return Vector2(1, 5);
        case MoveDirection::Left: return Vector2(-10, 1);
        case MoveDirection::Right: return Vector2(8, 1);
        }
    }
}

NetworkSwordEffect::NetworkSwordEffect(MoveDirection direction)
    : Super(GetSwordOffset(direction))
    , _timer(0.5f)
{
    AddComponent<SpriteRendererComponent>(CreateSwordSprite(ToVectorDirection(direction)));
}

void NetworkSwordEffect::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _timer.Tick(deltaTime);
    if (_timer.TimeOver())
    {
        Destroy();
    }
}
