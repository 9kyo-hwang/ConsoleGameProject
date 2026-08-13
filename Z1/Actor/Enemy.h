#pragma once
#include <Actor/Pawn.h>

class Enemy : public Pawn
{
    TYPE_DECLARATIONS(Enemy, Pawn)
public:
    Enemy(Craft::Vector2 position, int maxHp);
    ~Enemy() override;

    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void TakeDamage(int amount, const std::shared_ptr<Pawn>& instigator, const std::shared_ptr<Craft::Actor>& causer) override;

    // 목표 방향 계산 -> 이동은 Level에서
    Craft::Vector2 GetChaseDelta(const Craft::Vector2& target) const;

protected:
    void OnDeath(const std::shared_ptr<Pawn>& instigator) override;
};

