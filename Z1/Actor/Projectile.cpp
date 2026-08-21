#include "pch.h"
#include "Projectile.h"

#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

#include <Level/OverworldLevel.h>
#include <Level/DungeonLevel.h>
#include <Actor/Pawn.h>
#include <Actor/Player.h>
#include <Actor/Enemy.h>
#include <Engine/Engine.h>

#include <algorithm>

using namespace Craft;

Vector2 GetProjectileSpawnPosition(
    const Pawn& instigator,
    const ProjectileSpec& spec)
{
    const auto box = instigator.GetComponent<BoxComponent>();
    if (!box)
    {
        return instigator.GetWorldPosition() + spec.direction;
    }

    const Vector2 sourcePosition =
        instigator.GetWorldPosition() + box->GetOffset();

    const Vector2 sourceSize = box->GetSize();
    const Vector2 projectileSize(
        std::max(1, spec.boxSize.x),
        std::max(1, spec.boxSize.y)
    );

    const int sourceRight = sourcePosition.x + sourceSize.x - 1;
    const int sourceBottom = sourcePosition.y + sourceSize.y - 1;

    int spawnX = sourcePosition.x +
        (sourceSize.x - projectileSize.x) / 2;

    int spawnY = sourcePosition.y +
        (sourceSize.y - projectileSize.y) / 2;

    if (spec.direction.x > 0)
    {
        spawnX = sourceRight + 1;
    }
    else if (spec.direction.x < 0)
    {
        spawnX = sourcePosition.x - projectileSize.x;
    }

    if (spec.direction.y > 0)
    {
        spawnY = sourceBottom + 1;
    }
    else if (spec.direction.y < 0)
    {
        spawnY = sourcePosition.y - projectileSize.y;
    }

    return Vector2(spawnX, spawnY);
}

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

    if (!CanMoveTo(GetWorldPosition()))
    {
        Destroy();
        return;
    }

    // 한 프레임에 이동하는 칸 수만큼 검사
    const int moveSteps = ConsumeMoveSteps(deltaTime);
    for (int step = 0; step < moveSteps;++step)
    {
        const Vector2 next = GetWorldPosition() + _direction;
        if (!CanMoveTo(next))
        {
            Destroy();
            return;
        }

        SetPosition(next);
    }
}

bool Projectile::CanMoveTo(Vector2 destination) const
{
    const auto owner = GetOwner();

    if (auto overworld =
        std::dynamic_pointer_cast<OverworldLevel>(owner))
    {
        return overworld->CanProjectileOccupy(
            destination,
            *this
        );
    }

    if (auto dungeon =
        std::dynamic_pointer_cast<DungeonLevel>(owner))
    {
        return dungeon->CanProjectileOccupy(
            destination,
            *this
        );
    }

    return false;
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
                Engine::Get().PlayOneShot("Z1/LOZ_Shield.wav");
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
