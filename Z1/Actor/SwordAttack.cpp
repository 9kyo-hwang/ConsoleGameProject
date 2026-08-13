#include "pch.h"
#include "SwordAttack.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>
#include <Actor/Enemy.h>

using namespace Craft;

SwordAttack::SwordAttack(const Craft::Vector2& position, std::shared_ptr<Pawn> damageInstigator, int damage)
    : Super(position)
    , _damageInstigator(damageInstigator)
    , _damage(damage)
{
    AddComponent<SpriteRendererComponent>("=>");
    AddComponent<BoxComponent>(Vector2(2, 1));
}

void SwordAttack::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _lifetime -= deltaTime;
    if (_lifetime <= 0.f)
    {
        Destroy();
    }
}

void SwordAttack::OnCollision(const std::shared_ptr<Craft::Actor>& other)
{
    Super::OnCollision(other);

    if (_hasHit || !other)
    {
        return;
    }

    std::shared_ptr<Enemy> enemy = Cast<Enemy>(other);
    if (!enemy || enemy->IsDead())
    {
        return;
    }

    enemy->TakeDamage(_damage, _damageInstigator.lock(), shared_from_this());
    
    _hasHit = true;
    Destroy();
}
