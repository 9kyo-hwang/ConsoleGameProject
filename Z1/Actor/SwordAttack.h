#pragma once
#include <Actor/Actor.h>

class Pawn;

class SwordAttack : public Craft::Actor
{
    TYPE_DECLARATIONS(SwordAttack, Craft::Actor)

public:
    SwordAttack(const Craft::Vector2& position, std::shared_ptr<Pawn> damageInstigator, int damage);
    
    void Tick(float deltaTime) override;
    void OnCollision(const std::shared_ptr<Craft::Actor>& other) override;

private:
    std::weak_ptr<Pawn> _damageInstigator;
    float _lifetime = 0.5f;
    int _damage = 1;
    bool _hasHit = false;   // 1회만 공격하도록
};

