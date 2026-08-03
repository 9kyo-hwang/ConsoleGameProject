#pragma once
#include <Actor/Actor.h>

class Box : public Craft::Actor
{
    TYPE_DECLARATIONS(Box, Actor)
        
public:
    Box(const Craft::Vector2& position);
};

