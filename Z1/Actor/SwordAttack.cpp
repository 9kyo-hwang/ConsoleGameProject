#include "pch.h"
#include "SwordAttack.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>
#include <Actor/Enemy.h>
#include <Actor/Pawn.h>
#include <Actor/CombatEffect.h>
#include <Level/Level.h>

using namespace Craft;

namespace
{
    Vector2 GetAttackBoxSize(const Vector2& direction)
    {
        return direction.x != 0
            ? Vector2(2, 1)
            : Vector2(1, 2);
    }
}

SwordAttack::SwordAttack(const Craft::Vector2& position, std::shared_ptr<Pawn> damageInstigator, int damage)
    : Super(position)
    , _timer(0.5f)
    , _damageInstigator(damageInstigator)
    , _direction(position)
    , _damage(damage)
{
    const std::string image = _direction.x > 0 ? "=>"
        : _direction.x < 0 ? "<="
        : _direction.y < 0 ? "^"
        : "v";

    AddComponent<SpriteRendererComponent>(image);
    AddComponent<BoxComponent>(GetAttackBoxSize(_direction));
}

void SwordAttack::BeginPlay()
{
    Super::BeginPlay();

    const auto instigator = _damageInstigator.lock();
    const auto attackBox = GetComponent<BoxComponent>();
    const auto instigatorBox = instigator
        ? instigator->GetComponent<BoxComponent>()
        : nullptr;

    if (!instigator || !attackBox || !instigatorBox)
    {
        Destroy();
        return;
    }

    const Vector2 sourceOffset = instigatorBox->GetOffset();
    const Vector2 sourceSize = instigatorBox->GetSize();
    const Vector2 attackSize = attackBox->GetSize();

    Vector2 localPosition = sourceOffset;
    if (_direction.x > 0)
    {
        localPosition.x += sourceSize.x;
        localPosition.y += (sourceSize.y - attackSize.y) / 2;
    }
    else if (_direction.x < 0)
    {
        localPosition.x -= attackSize.x;
        localPosition.y += (sourceSize.y - attackSize.y) / 2;
    }
    else if (_direction.y < 0)
    {
        localPosition.x += (sourceSize.x - attackSize.x) / 2;
        localPosition.y -= attackSize.y;
    }
    else
    {
        localPosition.x += (sourceSize.x - attackSize.x) / 2;
        localPosition.y += sourceSize.y;
    }

    SetPosition(localPosition);

    const auto level = GetOwner();
    if (!level)
    {
        return;
    }

    level->SpawnActor<CombatEffect>(
        GetWorldPosition() + Vector2(-1, -1),
        CreateCombatEffectSprite(
            CombatEffectType::SwordSlash,
            _direction,
            Color::White
        ),
        0.12f,
        20
    );
}

void SwordAttack::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _timer.Tick(deltaTime);
    if (_timer.TimeOver())
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
