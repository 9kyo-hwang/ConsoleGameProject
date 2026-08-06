#include "pch.h"
#include "TransformComponent.h"

namespace Craft
{
    TransformComponent::TransformComponent(const Vector2& localPosition)
        : localPosition(localPosition)
        , previousWorldPosition(localPosition)  // TEMP
    {

    }

    Vector2 TransformComponent::GetWorldPosition() const
    {
        return localPosition;   // TEMP
    }

    void TransformComponent::SetWorldPosition(const Vector2& position)
    {
        localPosition = position;   // TEMP
    }

    void TransformComponent::SavePreviousWorldPosition()
    {
        previousWorldPosition = GetWorldPosition();
    }
}