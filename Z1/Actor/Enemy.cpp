#include "pch.h"
#include "Enemy.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

Enemy::Enemy(Vector2 position, int maxHp)
    : Super(position, maxHp)
{
    AddComponent<SpriteRendererComponent>("E", Color::Red, 11);
    AddComponent<BoxComponent>(Vector2::One);
}

Enemy::~Enemy()
{
}

void Enemy::BeginPlay()
{
    Super::BeginPlay();
}

void Enemy::Tick(float deltaTime)
{
    Super::Tick(deltaTime);
}

void Enemy::TakeDamage(int damageAmount, const std::shared_ptr<Pawn>& damageInstigator)
{
    Super::TakeDamage(damageAmount, damageInstigator);
}

void Enemy::OnDeath(const std::shared_ptr<Pawn>& damageInstigator)
{
    // Enemy는 파괴 처리

    Destroy();
}