#include "pch.h"
#include "SwordAttack.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

SwordAttack::SwordAttack(const Craft::Vector2& spawnPosition)
{
    AddComponent<SpriteRendererComponent>("=>");

}
