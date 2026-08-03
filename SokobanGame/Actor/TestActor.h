#pragma once
#include <Actor/Actor.h>

class TestActor : public Craft::Actor
{
    TYPE_DECLARATIONS(TestActor, Actor)

public:
    TestActor();
    
    void Tick(float deltaTime) override;
};