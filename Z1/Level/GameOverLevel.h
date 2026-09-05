#pragma once
#include <Level/Level.h>

class GameOverLevel : public Craft::Level
{
public:
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;
    void EndPlay() override;

private:
    bool _bgmStarted = false;
};

