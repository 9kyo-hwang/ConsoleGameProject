#pragma once
#include <Component/ActorComponent.h>
#include <string>
#include <Math/Color.h>

namespace Craft
{
    // Renderer가 RenderCommand를 생성하기 위해 필요한 데이터 공급자
    class CRAFT_API SpriteRendererComponent : public ActorComponent
    {
        TYPE_DECLARATIONS(SpriteRendererComponent, ActorComponent)
    
    public:
        SpriteRendererComponent(const std::string& image = "", Color color = Color::White, int sortingOrder = 0);
        ~SpriteRendererComponent() override = default;

        void Draw() override;

        inline const std::string& GetImage() const { return image; }
        inline void SetImage(const std::string& newImage) { image = newImage; }
        inline int GetWidth() const { return (int)image.size(); }

        inline Color GetColor() const { return color; }
        inline void SetColor(Color newColor) { color = newColor; }

        inline int GetSortingOrder() const { return sortingOrder; }
        inline void SetSortingOrder(int newOrder) { sortingOrder = newOrder; }

    protected:
        // Actor가 가지고 있던 이미지, 색상, draw order 정보 이관
        std::string image{};
        Color color = Color::White;
        int sortingOrder = 0;
    };
}