#include "pch.h"
#include "Enemy.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>
#include <Engine/Engine.h>
#include <Render/Sprite.h>

using namespace Craft;

Enemy::Enemy(Craft::Vector2 position, int maxHp, float moveSpeed, const std::string& image, Craft::Color color)
    : Super(position, maxHp, moveSpeed)
{
    auto sprite = Sprite::Create(image, color);
    AddComponent<SpriteRendererComponent>(sprite, 11);
    AddComponent<BoxComponent>(sprite->GetSize());
}

Enemy::~Enemy()
{
}

void Enemy::Think(float deltaTime, const Player& player)
{
    // 기본은 Player 추적
    desiredMove = GetChaseDelta(player.GetWorldPosition());
}

bool Enemy::ConsumeAttackRequest(EnemyAttackRequest& outRequest)
{
    if (!_attackRequest.has_value())
    {
        return false;
    }

    outRequest = std::move(*_attackRequest);
    _attackRequest.reset();

    return true;
}

int Enemy::TakeDamage(int amount, const std::shared_ptr<Pawn>& instigator, const std::shared_ptr<Craft::Actor>& causer)
{
    const int actualDamage =
        Super::TakeDamage(amount, instigator, causer);

    if (actualDamage > 0)
    {
        Engine::Get().PlayOneShot(
            IsDead()
            ? "Z1/LOZ_Enemy_Die.wav"
            : "Z1/LOZ_Enemy_Hit.wav"
        );
    }

    return actualDamage;
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
    Destroy();
}

void Enemy::RequestAttack(const EnemyAttackRequest& request)
{
    if (!_attackRequest.has_value())
    {
        _attackRequest = request;
    }
}

void Enemy::SetAppearance(
    const std::shared_ptr<const Sprite>& sprite,
    int sortingOrder)
{
    if (const auto renderer = GetComponent<SpriteRendererComponent>())
    {
        renderer->SetSprite(sprite);
        renderer->SetSortingOrder(sortingOrder);
    }

    if (const auto box = GetComponent<BoxComponent>())
    {
        box->SetSize(sprite->GetSize());
    }
}

int Enemy::GetVariantMaxHp(EnemyVariant variant)
{
    switch (variant)
    {
    case EnemyVariant::Red: return 1;
    case EnemyVariant::Blue: return 2;
    }

    return 1;
}

Color Enemy::GetVariantColor(EnemyVariant variant)
{
    switch (variant)
    {
    case EnemyVariant::Red: return Color::Red;
    case EnemyVariant::Blue: return Color::Blue;
    }

    return Color::Red;
}
