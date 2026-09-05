#pragma once

#include <Actor/Enemy.h>
#include <Util/Timer.h>

class Aquamentus : public Enemy
{
    TYPE_DECLARATIONS(Aquamentus, Enemy)

public:
    explicit Aquamentus(Craft::Vector2 position);

    void Think(float deltaTime, const Player& player) override;
    void OnMoveBlocked() override;
    int TakeDamage(
        int amount,
        const std::shared_ptr<Pawn>& instigator,
        const std::shared_ptr<Craft::Actor>& causer
    ) override;
    int GetContactDamage() const override { return 2; }

private:
    void RequestFireballAttack(const Player& player);

private:
    Craft::Vector2 _moveDirection = Craft::Vector2::Right * -1;
    Timer _attackTimer;
};
