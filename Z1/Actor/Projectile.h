#pragma once
#include <Actor/Actor.h>
#include <Math/Vector2.h>
#include <Util/Timer.h>

class Pawn;

enum class ProjectileType
{
    SwordBeam,  // link
    Rock,
    Spear,
    Fireball,
};

enum class ProjectileFaction
{
    Player,
    Enemy
};

struct ProjectileSpec
{
    ProjectileType type = ProjectileType::SwordBeam;
    ProjectileFaction faction = ProjectileFaction::Enemy;

    int damage = 1;

    float speed = 30.f;
    float lifetime = 2.f;

    Craft::Vector2 direction;
    Craft::Vector2 boxSize = Craft::Vector2::One;

    std::string image = "*";
    Craft::Color color = Craft::Color::White;
    int sortingOrder = 11;
};

Craft::Vector2 GetProjectileSpawnPosition(
    const Pawn& instigator,
    const ProjectileSpec& spec
);

class Projectile : public Craft::Actor
{
    TYPE_DECLARATIONS(Projectile, Craft::Actor)

public:
    Projectile(Craft::Vector2 position, const ProjectileSpec& spec, const std::shared_ptr<Pawn>& instigator);

    void Tick(float deltaTime) override;
    void OnCollision(const std::shared_ptr<Craft::Actor>& other) override;

    int ConsumeMoveSteps(float deltaTime);
    void ClearMoveRemainder();

public:
    inline ProjectileType Type() const { return _spec.type; }
    inline ProjectileFaction Faction() const { return _spec.faction; }
    inline Craft::Vector2 Direction() const { return _spec.direction; }
    inline int Damage() const { return _spec.damage; }
    inline bool Shieldable() const
    {
        return _spec.type == ProjectileType::Rock ||
               _spec.type == ProjectileType::Spear ||
               _spec.type == ProjectileType::Fireball;
    }

protected:
    bool CanHit(const std::shared_ptr<Pawn>& target) const;

private:
    bool CanMoveTo(Craft::Vector2 destination) const;

private:
    ProjectileSpec _spec;
    std::weak_ptr<Pawn> _instigator;

    Craft::Vector2 _direction;

    float _moveRemainder;
    Timer _timer;

    bool _hasHit;
};

