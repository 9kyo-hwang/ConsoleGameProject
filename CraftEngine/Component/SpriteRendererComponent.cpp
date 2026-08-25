#include "pch.h"
#include "SpriteRendererComponent.h"
#include <Component/TransformComponent.h>
#include <Render/Renderer.h>
#include <Actor/Actor.h>

namespace Craft
{
    SpriteRendererComponent::SpriteRendererComponent(std::shared_ptr<const Sprite> sprite, int sortingOrder)
        : sprite(std::move(sprite))
        , sortingOrder(sortingOrder)
    {
    }

    void SpriteRendererComponent::Draw()
    {
        Super::Draw();  // 실제로는 하는 거 없음

        auto actor = GetOwner();
        if (!actor || !actor->IsActive())
        {
            return;
        }

        auto transform = actor->GetTransform();
        if (!transform)
        {
            return;
        }

        const Vector2 worldPosition = transform->GetWorldPosition();
        if (!sprite)
        {
            return;
        }

        Renderer::Get().SubmitWorld(sprite, worldPosition, sortingOrder);
    }
}