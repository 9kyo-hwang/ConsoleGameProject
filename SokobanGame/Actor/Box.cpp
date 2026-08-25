#include <pch.h>
#include "Box.h"

#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

Box::Box(const Craft::Vector2& position)
    : Super(position)
{
    AddComponent<SpriteRendererComponent>(Sprite::Create("B", Color::Red), 3);
    AddComponent<BoxComponent>(1);  // width = 1
}
