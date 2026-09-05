#pragma once
#include <Actor/Actor.h>

class EnemyBullet : public Craft::Actor
{
    TYPE_DECLARATIONS(EnemyBullet, Craft::Actor)

public:
    EnemyBullet(const Craft::Vector2& startPosition, float moveSpeed = 15.f);
    ~EnemyBullet() override = default;

private:
    void Tick(float deltaTime) override;

    float _moveSpeed = 0.f;
    float _posY = 0.f;
};

