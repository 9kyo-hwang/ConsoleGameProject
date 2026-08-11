#pragma once
#include <Component/ActorComponent.h>
#include <string>
#include <Render/Sprite.h>
#include <Math/Color.h>
#include <utility>
#include <memory>

namespace Craft
{
    // Renderer가 RenderCommand를 생성하기 위해 필요한 데이터 공급자
    class CRAFT_API SpriteRendererComponent : public ActorComponent
    {
        TYPE_DECLARATIONS(SpriteRendererComponent, ActorComponent)
    
    public:
        SpriteRendererComponent(const std::string& image = "", Color color = Color::White, int sortingOrder = 0);
        SpriteRendererComponent(std::shared_ptr<const Sprite> sprite, int sortingOrder = 0);
        ~SpriteRendererComponent() override = default;

        void Draw() override;

        inline void SetImage(const std::string& newImage) { image = newImage; sprite.reset(); }
        inline const std::string& GetImage() const { return image; }
        int GetWidth() const;

        inline Color GetColor() const { return color; }
        inline void SetColor(Color newColor) { color = newColor; }

        inline int GetSortingOrder() const { return sortingOrder; }
        inline void SetSortingOrder(int newOrder) { sortingOrder = newOrder; }

        inline void SetSprite(std::shared_ptr<const Sprite> newSprite) { sprite = std::move(newSprite); image.clear(); }
        inline const Sprite* GetSprite() const { return sprite.get(); }
        Vector2 GetSpriteSize() const;

        inline void SetCellScale(const Vector2& newScale) { assert(newScale.x > 0 && newScale.y > 0); cellScale = newScale; }
        inline Vector2 GetCellScale() const { return cellScale; }

    protected:
        // Actor가 가지고 있던 이미지, 색상, draw order 정보 이관
        std::string image{};
        Color color = Color::White;
        int sortingOrder = 0;
        std::shared_ptr<const Sprite> sprite;  // 실질 소유, renderer는 참조.
        Vector2 cellScale = Vector2::One;
    };
}