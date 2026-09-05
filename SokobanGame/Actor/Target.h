#pragma once
#include <Actor/Actor.h>

class Target : public Craft::Actor
{
    TYPE_DECLARATIONS(Target, Actor)

public:
    Target(const Craft::Vector2& position);
};

