#pragma once
#include <Level/Level.h>

class CollisionTestActor;

class DevelopmentLevel : public Craft::Level
{
public:
    DevelopmentLevel();

    void OnInitialized() override;
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;

private:
    std::shared_ptr<CollisionTestActor> _testA;
    std::shared_ptr<CollisionTestActor> _testB;
};

