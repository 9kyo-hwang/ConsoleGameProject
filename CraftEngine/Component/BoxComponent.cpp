#include "pch.h"
#include "BoxComponent.h"

namespace Craft
{
    BoxComponent::BoxComponent(const Vector2& size, const Vector2& offset)
        : size(size)
        , offset(offset)
    {
    }
}