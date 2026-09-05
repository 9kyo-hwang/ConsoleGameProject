#pragma once
#include <Actor/Enemy.h>
#include <Util/Timer.h>

class Tektite : public Enemy
{
    TYPE_DECLARATIONS(Tektite, Enemy)
public:
    Tektite(Craft::Vector2 position, EnemyVariant variant);

    void Think(float deltaTime, const Player& player) override;
    void OnMoveBlocked() override;

private:
    Craft::Vector2 GetHopDirection(const Player& player) const;

private:
    Craft::Vector2 _hopDirection = Craft::Vector2::Zero;
    bool _isHopping = false;

    Timer _hopCooldown;
    Timer _hopDuration;
};
