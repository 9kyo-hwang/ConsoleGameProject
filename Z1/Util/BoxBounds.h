#pragma once

#include <Math/Vector2.h>

struct BoxBounds
{
public:
    BoxBounds(Craft::Vector2 position = Craft::Vector2::Zero, Craft::Vector2 size = Craft::Vector2::Zero)
        : _position(position)
        , _size(size)
    {

    }

    inline bool IsValid() const { return _size.x > 0 && _size.y > 0; }
    inline int Left() const { return _position.x; }
    inline int Top() const { return _position.y; }
    inline int Right() const { return _position.x + _size.x - 1; }
    inline int Bottom() const { return _position.y + _size.y - 1; }

    bool Overlaps(const BoxBounds& other) const
    {
        if (!IsValid() || !other.IsValid())
        {
            return false;
        }

        return !(Right() < other.Left() ||
                 other.Right() < Left() ||
                 Bottom() < other.Top() ||
                 other.Bottom() < Top());
    }

    bool IsSideContact(const BoxBounds& other) const
    {
        if (!IsValid() || !other.IsValid())
        {
            return false;
        }

        const bool horizontalOverlap =
            Left() <= other.Right() &&
            other.Left() <= Right();

        const bool verticalOverlap =
            Top() <= other.Bottom() &&
            other.Top() <= Bottom();

        const bool horizontallyAdjacent =
            Right() + 1 >= other.Left() &&
            other.Right() + 1 >= Left();

        const bool verticallyAdjacent =
            Bottom() + 1 >= other.Top() &&
            other.Bottom() + 1 >= Top();

        return (horizontallyAdjacent && verticalOverlap) ||
               (verticallyAdjacent && horizontalOverlap);
    }

    bool IsInside(const BoxBounds& container) const
    {
        return IsValid() &&
               container.IsValid() &&
               Left() >= container.Left() &&
               Top() >= container.Top() &&
               Right() <= container.Right() &&
               Bottom() <= container.Bottom();
    }

private:
    Craft::Vector2 _position;
    Craft::Vector2 _size;
};
