#pragma once
#include <Actor/Enemy.h>
#include <Util/Timer.h>

class Moblin : public Enemy
{
    TYPE_DECLARATIONS(Moblin, Enemy)
public:
    Moblin(Craft::Vector2 position, EnemyVariant variant);

    void Think(float deltaTime, const Player& player) override;
    void OnMoveBlocked() override;

private:
    void RequestSpearAttack();

private:
    EnemyVariant _variant;
    Craft::Vector2 _direction = Craft::Vector2::Right;

    Timer _directionTimer;
    Timer _attackTimer;
};
