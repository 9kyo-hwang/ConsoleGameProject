#pragma once
#include <Actor/Actor.h>

class Wall : public Craft::Actor
{
    TYPE_DECLARATIONS(Wall, Actor)

public:
    Wall(const Craft::Vector2& position);
};

