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
    void TakeDamage(int damageAmount, const std::shared_ptr<Pawn>& damageInstigator) override;

protected:
    void OnDeath(const std::shared_ptr<Pawn>& damageInstigator) override;

private:

};

