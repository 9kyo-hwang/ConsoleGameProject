#pragma once
#include <Actor/Actor.h>

class SwordAttack : public Craft::Actor
{
    TYPE_DECLARATIONS(SwordAttack, Craft::Actor)

public:
    SwordAttack(const Craft::Vector2& spawnPosition);

};

