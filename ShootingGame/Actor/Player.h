#pragma once
#include <Actor/Actor.h>

class Player : public Craft::Actor
{
    TYPE_DECLARATIONS(Player, Craft::Actor)

public:
    Player();

private:
    void Tick(float deltaTime) override;
    void Move(float direction, float deltaTime);

private:
    float _posX = 0.f;
    float _moveSpeed = 70.f;
};

