#include "pch.h"
#include "Projectile.h"

#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

#include <Level/OverworldLevel.h>
#include <Actor/Pawn.h>
#include <Actor/Player.h>
#include <Actor/Enemy.h>

using namespace Craft;

Projectile::Projectile(Craft::Vector2 position, const ProjectileSpec& spec, const std::shared_ptr<Pawn>& instigator)
    : Super(position)
    , _spec(spec)
    , _instigator(instigator)
    , _direction(spec.direction)
    , _timer(spec.lifetime)
    , _hasHit(false)
{
    AddComponent<SpriteRendererComponent>(_spec.image, _spec.color, _spec.sortingOrder);
    AddComponent<BoxComponent>(_spec.boxSize);
}

void Projectile::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    if (!IsActive())
    {
        return;
    }

    _timer.Tick(deltaTime);
    if (_timer.TimeOver())
    {
        Destroy();
        return;
    }

    auto level = std::dynamic_pointer_cast<OverworldLevel>(GetOwner());
    if (!level)
    {
        Destroy();
        return;
    }

    if (!level->CanProjectileOccupy(GetWorldPosition(), *this))
    {
        Destroy();
        return;
    }

    // 한 프레임에 이동하는 칸 수만큼 검사
    const int moveSteps = ConsumeMoveSteps(deltaTime);
    for (int step = 0; step < moveSteps;++step)
    {
        const Vector2 next = GetWorldPosition() + _direction;
        if (!level->CanProjectileOccupy(next, *this))
        {
            Destroy();
            return;
        }

        SetPosition(next);
    }
}

void Projectile::OnCollision(const std::shared_ptr<Craft::Actor>& other)
{
    Super::OnCollision(other);

    if (!IsActive() || _hasHit || !other)
    {
        return;
    }

    auto target = Cast<Pawn>(other);
    if (!CanHit(target))
    {
        return;
    }

    // 적 Projectile이 Player 맞춘 경우 방패 판정 수행
    if (_spec.faction == ProjectileFaction::Enemy)
    {
        if (auto player = Cast<Player>(target))
        {
            if (player->Shieldable(*this))
            {
                _hasHit = true;
                Destroy();
                return;
            }
        }
    }

    int actualDamage = target->TakeDamage(_spec.damage, _instigator.lock(), shared_from_this());
    if (actualDamage > 0)
    {
        _hasHit = true;
        Destroy();
    }
}

int Projectile::ConsumeMoveSteps(float deltaTime)
{
    _moveRemainder += _spec.speed * deltaTime;

    const int steps = (int)_moveRemainder;
    _moveRemainder -= steps;

    return steps;
}

void Projectile::ClearMoveRemainder()
{
    _moveRemainder = 0.f;
}

bool Projectile::CanHit(const std::shared_ptr<Pawn>& target) const
{
    if (!target || target->IsDead())
    {
        return false;
    }

    // 발사자 자신
    if (target.get() == _instigator.lock().get())
    {
        return false;
    }

    if (_spec.faction == ProjectileFaction::Player)
    {
        return target->IsA<Enemy>();
    }

    if (_spec.faction == ProjectileFaction::Enemy)
    {
        return target->IsA<Player>();
    }

    return false;
}
