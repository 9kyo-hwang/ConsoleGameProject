#include "pch.h"
#include "BoxComponent.h"

namespace Craft
{
    BoxComponent::BoxComponent(const Vector2& size, const Vector2& offset)
        : size(size)
        , offset(offset)
    {
    }

    BoxComponent::BoxComponent(int width)
        : size(width, 1)
        , offset(Vector2::Zero)
    {
    }
}