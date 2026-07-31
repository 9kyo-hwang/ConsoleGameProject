#pragma once
#include "Actor.h"

class TestActor : public Craft::Actor
{
public:
    void Tick(float deltaTime) override;
};