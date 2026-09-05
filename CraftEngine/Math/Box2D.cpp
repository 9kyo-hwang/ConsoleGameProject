#include "pch.h"
#include "Box2D.h"

namespace Craft
{
    bool Box2D::Overlaps(const Box2D& other) const
    {
        if (!IsValid() || !other.IsValid())
        {
            return false;
        }

        return !(
            Right() < other.Left() ||
            other.Right() < Left() ||
            Bottom() < other.Top() ||
            other.Bottom() < Top()
        );
    }

    bool Box2D::IsSideContact(const Box2D& other) const
    {
        if (!IsValid() || !other.IsValid())
        {
            return false;
        }

        const bool horizontalOverlap = Left() <= other.Right() && other.Left() <= Right();
        const bool verticalOverlap = Top() <= other.Bottom() && other.Top() <= Bottom();

        const bool horizontallyAdjacent = Right() + 1 >= other.Left() && other.Right() + 1 >= Left();
        const bool verticallyAdjacent = Bottom() + 1 >= other.Top() && other.Bottom() + 1 >= Top();

        return (horizontallyAdjacent && verticalOverlap) || (verticallyAdjacent && horizontalOverlap);
    }

    bool Box2D::IsInside(const Box2D& container) const
    {
        return IsValid() && container.IsValid() &&
            Left() >= container.Left() &&
            Top() >= container.Top() &&
            Right() <= container.Right() &&
            Bottom() <= container.Bottom();
    }
}
