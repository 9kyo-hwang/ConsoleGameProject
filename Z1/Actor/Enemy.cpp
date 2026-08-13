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

Vector2 Enemy::GetChaseDelta(const Vector2& target) const
{
    const Vector2 distance = target - GetWorldPosition();
    if (std::abs(distance.x) >= std::abs(distance.y))
    {
        return distance.x >= 0 ? Vector2::Right : Vector2::Right * -1;
    }

    return distance.y >= 0 ? Vector2::Up * -1: Vector2::Up;
}

int Enemy::ConsumeMoveSteps(float deltaTime)
{
    _moveRemainder += _moveSpeed * deltaTime;

    const int steps = (int)_moveRemainder;
    _moveRemainder -= steps;

    return steps;
}

void Enemy::ClearMoveRemainder()
{
    _moveRemainder = 0.f;
}

void Enemy::MoveBy(const Craft::Vector2& delta)
{
    SetPosition(GetPosition() + delta);
}

void Enemy::OnDeath(const std::shared_ptr<Pawn>& damageInstigator)
{
    // Enemy는 파괴 처리

    Destroy();
}