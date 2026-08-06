#include "pch.h"
#include "SpriteRendererComponent.h"

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
    }
}