#include "pch.h"
#include "Enemy.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

Enemy::Enemy(Vector2 position, int maxHp)
    : Super(position, maxHp, 8.f)
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

void Enemy::TakeDamage(int amount, const std::shared_ptr<Pawn>& instigator, const std::shared_ptr<Craft::Actor>& causer)
{
    Super::TakeDamage(amount, instigator, causer);
}

Vector2 Enemy::GetChaseDelta(const Vector2& target) const
{
    const Vector2 distance = target - GetWorldPosition();
    if (std::abs(distance.x) >= std::abs(distance.y))
    {
        return distance.x >= 0 ? Vector2::Right : Vector2::Right * -1;
    }

    return distance.y >= 0 ? Vector2::Up * -1: Vector2::Up;
}

void Enemy::OnDeath(const std::shared_ptr<Pawn>& instigator)
{
    // TODO: 사망 이펙트 + 사운드 처리

    Destroy();
}