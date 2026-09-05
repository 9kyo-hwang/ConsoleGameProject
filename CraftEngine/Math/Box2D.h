#pragma once
#include <Core/Core.h>
#include <Math/Vector2.h>

/*
* 특정 위치(position)에 배치된 사각형(크기: size) 기반 AABB 판정
*/
namespace Craft
{
    struct CRAFT_API Box2D
    {
        Vector2 position;
        Vector2 size;

        inline bool IsValid() const { return size.x > 0 && size.y > 0; }
        inline int Left() const { return position.x; }
        inline int Top() const { return position.y; }
        inline int Right() const { return position.x + size.x - 1; }
        inline int Bottom() const { return position.y + size.y - 1; }

        bool Overlaps(const Box2D& other) const;
        bool IsSideContact(const Box2D& other) const;
        bool IsInside(const Box2D& container) const;
    };
}