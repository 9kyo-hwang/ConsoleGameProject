#pragma once

#include <Math/Vector2.h>

#include <memory>

namespace Craft
{
    class Sprite;
}

std::shared_ptr<const Craft::Sprite> CreateSwordSprite(const Craft::Vector2& direction);
