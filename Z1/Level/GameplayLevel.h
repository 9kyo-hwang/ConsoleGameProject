#pragma once
#include <Level/Level.h>

class GameplayLevel : public Craft::Level
{
public:
    GameplayLevel();
    ~GameplayLevel() override = default;

    void Tick(float deltaTime) override;
    void Draw() override;
};

