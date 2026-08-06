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

    void SpriteRendererComponent::Draw()
    {
        Super::Draw();  // 실제로는 하는 거 없음

        // TODO: Actor에 연결 + Transform Getter 준비되면 구현
        // Renderer::Submit()이 position 정보도 같이 받고 있기 때문

        std::shared_ptr<Actor> actor = GetOwner();
        if (!actor || !actor->IsActive())
        {
            return;
        }

        auto transform = actor->GetComponent<TransformComponent>();
        if (!transform)
        {
            return;
        }

        // Renderer는 콘솔 창에 그리는 역할이므로, WorldPosition이 필요
        Renderer::Get().Submit(image, transform->GetWorldPosition(), color, sortingOrder);
    }
}