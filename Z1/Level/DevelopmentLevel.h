#pragma once
#include <Level/Level.h>

class Player;

class DevelopmentLevel : public Craft::Level
{
public:
    DevelopmentLevel();

    void OnInitialized() override;
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;

private:
    std::string _mapStatus = "NOT TESTED";
    bool _attempted = false;
};

