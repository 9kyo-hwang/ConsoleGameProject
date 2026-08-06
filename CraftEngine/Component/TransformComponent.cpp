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
        if (auto parent = GetParent())
        {
            // ex) 3, 0 + 1, 2 -> 4, 2
            return parent->GetWorldPosition() + localPosition;
        }

        return localPosition;
    }

    void TransformComponent::SetWorldPosition(const Vector2& position)
    {
        if (auto parent = GetParent())
        {
            localPosition = position - parent->GetWorldPosition();
            return;
        }

        localPosition = position;
    }

    void TransformComponent::SavePreviousWorldPosition()
    {
        previousWorldPosition = GetWorldPosition();
    }
}