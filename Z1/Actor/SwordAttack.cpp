#include "pch.h"
#include "SwordAttack.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

SwordAttack::SwordAttack(const Craft::Vector2& spawnPosition)
    : Super(spawnPosition)
{
    AddComponent<SpriteRendererComponent>("=>");
    AddComponent<BoxComponent>(Vector2(2, 1));
}

void SwordAttack::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _lifetime -= deltaTime;
    if (_lifetime <= 0.f)
    {
        Destroy();
    }
}
