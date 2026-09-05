#pragma once
#include <Math/Vector2.h>

class ICanPlayerMove
{
public:
    virtual bool CanMoveTo(const Craft::Vector2& from, const Craft::Vector2& to) = 0;
};