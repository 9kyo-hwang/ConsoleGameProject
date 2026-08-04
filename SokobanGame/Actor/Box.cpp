#include <pch.h>
#include "Box.h"

Box::Box(const Craft::Vector2& position)
    : Super("B", position, Craft::Color::Red)
{
    // Ground나 Target랑 겹쳤을 때 위에 보이도록
    sortingOrder = 3;
}
