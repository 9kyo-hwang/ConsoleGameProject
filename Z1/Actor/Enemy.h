#pragma once
#include <Actor/Pawn.h>
#include <Actor/EnemyTypes.h>
#include <Actor/Projectile.h>
#include <Actor/Player.h>

#include <optional>
#include <vector>

struct EnemyAttackRequest
{
    ProjectileSpec projectile;
    Craft::Vector2 spawnOffset = Craft::Vector2::Zero;
    std::vector<ProjectileSpec> projectiles;
};

class Enemy : public Pawn
{
    TYPE_DECLARATIONS(Enemy, Pawn)
public:
    Enemy(Craft::Vector2 position, int maxHp, float moveSpeed = 8.f, const std::string& image = "E", Craft::Color color = Craft::Color::Red);
    ~Enemy() override;

    virtual void Think(float deltaTime, const Player& player);  // 어떻게 움직일 것인가
    virtual Craft::Vector2 GetDesiredMove() const { return desiredMove; }
    virtual void OnMoveBlocked() {}
    virtual int GetContactDamage() const { return 1; }

    bool ConsumeAttackRequest(EnemyAttackRequest& outRequest);

    int TakeDamage(int amount, const std::shared_ptr<Pawn>& instigator, const std::shared_ptr<Craft::Actor>& causer) override;

    // 목표 방향 계산 -> 이동은 Level에서
    Craft::Vector2 GetChaseDelta(const Craft::Vector2& target) const;

protected:
    void OnDeath(const std::shared_ptr<Pawn>& instigator) override;
    void RequestAttack(const EnemyAttackRequest& request);

protected:
    Craft::Vector2 desiredMove = Craft::Vector2::Zero;

private:
    std::optional<EnemyAttackRequest> _attackRequest;
};

