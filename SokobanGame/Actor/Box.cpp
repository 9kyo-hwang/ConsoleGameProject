#include <pch.h>
#include "Box.h"

#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

Box::Box(const Craft::Vector2& position)
    : Super(position)
{
    auto sprite = Sprite::Create("B", Color::Red);
    AddComponent<SpriteRendererComponent>(sprite, 3);
    AddComponent<BoxComponent>(sprite->GetSize());
}
