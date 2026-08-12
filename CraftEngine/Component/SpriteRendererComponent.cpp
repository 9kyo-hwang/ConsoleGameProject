#include "pch.h"
#include "SpriteRendererComponent.h"
#include <Component/TransformComponent.h>
#include <Render/Renderer.h>
#include <Actor/Actor.h>

namespace Craft
{
    SpriteRendererComponent::SpriteRendererComponent(const std::string& image, Color color, int sortingOrder)
        : image(image)
        , color(color)
        , sortingOrder(sortingOrder)
    {
    }

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
        if (sprite)
        {
            Renderer::Get().SubmitWorld(
                sprite,
                worldPosition,
                sortingOrder
            );
        }
        else
        {
            Renderer::Get().SubmitWorld(
                image, 
                worldPosition,
                color,
                sortingOrder
            );
        }
    }

    int SpriteRendererComponent::GetWidth() const
    {
        if (sprite)
        {
            return sprite->GetSize().x;
        }

        return static_cast<int>(image.size());
    }

    Vector2 SpriteRendererComponent::GetSpriteSize() const
    {
        if (sprite)
        {
            return sprite->GetSize();
        }

        return Vector2(static_cast<int>(image.size()), 1);
    }
}