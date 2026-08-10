#pragma once
#include <Component/ActorComponent.h>
#include <Math/Vector2.h>

namespace Craft
{
    // AABB 기반 충돌 검사를 위한 데이터 제공자
    class CRAFT_API BoxComponent : public ActorComponent
    {
        TYPE_DECLARATIONS(BoxComponent, ActorComponent)

    public:
        BoxComponent(const Vector2& size, const Vector2& offset = Vector2::Zero);
        BoxComponent(int width = 0);
        ~BoxComponent() override = default;

        inline Vector2 GetSize() const { return size; }
        inline void SetSize(const Vector2& newSize) { size = newSize; }

        inline Vector2 GetOffset() const { return offset; }
        inline void SetOffset(const Vector2& newOffset) { offset = newOffset; }

        inline int GetWidth() const { return size.x; }
        inline void SetWidth(int newWidth) { size.x = newWidth; }

    protected:
        // 2차원으로 확장, [Actor 월드 위치 + Offset + Size]를 고려
        Vector2 size;
        Vector2 offset;
    };
}
