#pragma once
#include <Level/Level.h>

class GameOverLevel : public Craft::Level
{
public:
    void Tick(float deltaTime) override;
    void Draw() override;
};

