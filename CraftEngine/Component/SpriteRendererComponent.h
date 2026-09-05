#pragma once
#include <Component/ActorComponent.h>
#include <Render/Sprite.h>
#include <utility>
#include <memory>

namespace Craft
{
    // Renderer가 RenderCommand를 생성하기 위해 필요한 데이터 공급자
    class CRAFT_API SpriteRendererComponent : public ActorComponent
    {
        TYPE_DECLARATIONS(SpriteRendererComponent, ActorComponent)
    
    public:
        SpriteRendererComponent(std::shared_ptr<const Sprite> sprite, int sortingOrder = 0);
        ~SpriteRendererComponent() override = default;

        void Draw() override;

        inline int GetSortingOrder() const { return sortingOrder; }
        inline void SetSortingOrder(int newOrder) { sortingOrder = newOrder; }

        inline void SetSprite(std::shared_ptr<const Sprite> newSprite) { sprite = std::move(newSprite); }
        inline const Sprite* GetSprite() const { return sprite.get(); }

    protected:
        int sortingOrder = 0;
        std::shared_ptr<const Sprite> sprite;  // 실질 소유, renderer는 참조.
    };
}