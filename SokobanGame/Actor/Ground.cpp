#include <pch.h>
#include "Ground.h"
#include <Component/SpriteRendererComponent.h>

using namespace Craft;

Ground::Ground(const Craft::Vector2& position)
    : Super(position)
{
    AddComponent<SpriteRendererComponent>(Sprite::Create(" ", Color::Black));
}
