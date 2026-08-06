#pragma once
#include <Component/ActorComponent.h>

namespace Craft
{
    // AABB 기반 충돌 검사를 위한 너비 데이터 제공자
    class CRAFT_API BoxComponent : public ActorComponent
    {
        TYPE_DECLARATIONS(BoxComponent, ActorComponent)

    public:
        BoxComponent(int width = 0);
        ~BoxComponent() override = default;

        inline int GetWidth() const { return width; }
        inline void SetWidth(int newWidth) { width = newWidth; }

    protected:
        int width = 0;
    };
}
