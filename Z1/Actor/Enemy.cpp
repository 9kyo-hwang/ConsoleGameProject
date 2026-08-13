#include "pch.h"
#include "Enemy.h"

using namespace Craft;

Enemy::Enemy(Vector2 position, int maxHp)
    : Super(position, maxHp)
{
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

void Enemy::OnDeath(const std::shared_ptr<Actor>& damageInstigator)
{
    // Enemy는 파괴 처리

    if (IsDead()) return;
    Super::Destroy();
}