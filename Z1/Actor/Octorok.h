#pragma once
#include <Actor/Enemy.h>
#include <Util/Timer.h>

class Octorok : public Enemy
{
    TYPE_DECLARATIONS(Octorok, Enemy)
public:
    Octorok(Craft::Vector2 position, EnemyVariant variant);

    void Think(float deltaTime, const Player& player) override;
    void OnMoveBlocked() override { Rotate(); }

private:
    Craft::Vector2 GetRandomDirection() const;
    void Rotate();
    void RequestRockAttack();

private:
    EnemyVariant _variant;

    Craft::Vector2 _direction = Craft::Vector2::Right;

    Timer _rotateTimer;
    Timer _attackTimer;
};

