#pragma once
#include <Level/Level.h>

class ClearLevel : public Craft::Level
{
public:
    ClearLevel();
    ~ClearLevel() override = default;

    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;
    void EndPlay() override;

private:
    bool _bgmStarted = false;
};

