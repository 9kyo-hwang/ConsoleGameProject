#pragma once
#include <Level/Level.h>

class TitleLevel : public Craft::Level
{
public:
    TitleLevel();
    ~TitleLevel() override = default;

    void Tick(float deltaTime) override;
    void Draw() override;
};

