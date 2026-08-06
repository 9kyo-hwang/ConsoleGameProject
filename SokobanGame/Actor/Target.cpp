#include <pch.h>
#include "Target.h"
#include <Component/SpriteRendererComponent.h>

using namespace Craft;

Target::Target(const Craft::Vector2& position)
    : Super(position)
{
    AddComponent<SpriteRendererComponent>("T", Craft::Color::Blue, 1);
}
