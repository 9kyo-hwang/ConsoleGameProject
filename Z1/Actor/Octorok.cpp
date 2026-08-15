#include "pch.h"
#include "Octorok.h"

#include <Math/MathUtility.h>

using namespace Craft;

namespace
{
    int MaxHp(EnemyVariant variant)
    {
        switch (variant)
        {
        case EnemyVariant::Red: return 1;
        case EnemyVariant::Blue: return 2;
        }
    }

    Color GetColor(EnemyVariant variant)
    {
        switch (variant)
        {
        case EnemyVariant::Red: return Color::Red;
        case EnemyVariant::Blue: return Color::Blue;
        }
    }
}

Octorok::Octorok(Craft::Vector2 position, EnemyVariant variant)
    : Super(position, MaxHp(variant), 8.f, "O", GetColor(variant))
    , _variant(variant)
    , _rotateTimer(0.75f)
    , _attackTimer(1.5f)
{
    Rotate();
}

void Octorok::Think(float deltaTime, const Player&)
{
    _rotateTimer.Tick(deltaTime);
    _attackTimer.Tick(deltaTime);

    if (_rotateTimer.TimeOver())
    {
        _rotateTimer.Reset();
        Rotate();
    }

    desiredMove = _direction;

    if (_attackTimer.TimeOver())
    {
        _attackTimer.Reset();
        RequestRockAttack();
    }
}

Craft::Vector2 Octorok::GetRandomDirection() const
{
    switch (FMath::RandRange(0, 3))
    {
    case 0: return Vector2::Up;
    case 1: return Vector2::Right;
    case 2: return Vector2::Up * -1;
    case 3: return Vector2::Right * -1;
    }
}

void Octorok::Rotate()
{
    _direction = GetRandomDirection();
}

void Octorok::RequestRockAttack()
{
    // 바라보는 방향으로 공격

    EnemyAttackRequest request;

    request.projectile = ProjectileSpec
    {
        .type = ProjectileType::Rock,
        .faction = ProjectileFaction::Enemy,
        .damage = 1,
        .speed = 20.f,
        .lifetime = 2.f,
        .direction = _direction,
        .boxSize = Vector2::One,
        .image = "o",
        .color = Color::DarkYellow,
        .sortingOrder = 12
    };

    request.spawnOffset = _direction;

    RequestAttack(request);
}
