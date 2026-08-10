#pragma once
#include <Level/Level.h>

class ClearLevel : public Craft::Level
{
public:
    ClearLevel();
    ~ClearLevel() override = default;
    
    void Tick(float deltaTime) override;
    void Draw() override;
};

