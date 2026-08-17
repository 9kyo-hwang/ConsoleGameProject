#pragma once
#include <Actor/Actor.h>
#include <Util/Timer.h>

enum class Facing
{
    Up = 0,
    Right,
    Down,
    Left,
    NONE
};

/*
* 피격 여부, 무적 시간, 넉백 처리를 위한 정보 보관
*/
class Pawn : public Craft::Actor
{
    TYPE_DECLARATIONS(Pawn, Craft::Actor)

public:
    Pawn(Craft::Vector2 position, int maxHp, float moveSpeed);
    ~Pawn() override;

    void Tick(float deltaTime) override;
    void Draw() override;

    virtual int TakeDamage(int amount, const std::shared_ptr<Pawn>& instigator, const std::shared_ptr<Craft::Actor>& causer);

    void SetHealth(int health);
    void RestoreFullHealth();

    int ConsumeMoveSteps(float deltaTime);
    void ClearMoveRemainder();
    void MoveBy(const Craft::Vector2& delta);

    int ConsumeKnockbackSteps(float deltaTime);
    void StopKnockback();
    
public:
    inline int GetHp() const { return _hp; }
    inline int GetMaxHp() const { return _maxHp; }
    inline bool IsDead() const { return _hp <= 0; }
    inline bool IsFullHp() const { return _hp == _maxHp; }

    inline Facing GetFacing() const { return _facing; }
    inline void SetFacing(Facing facing) { _facing = facing; }

    Craft::Vector2 GetFacingDirection() const;

    inline bool IsKnockback() const { return _remainKnockbackSteps > 0; }
    inline Craft::Vector2 GetKnockbackDirection() const { return _knockbackDirection; }

protected:
    virtual void OnDeath(const std::shared_ptr<Pawn>& instigator) {}
    inline void SetMoveSpeed(float moveSpeed) { _moveSpeed = moveSpeed; }

private:
    void Knockback(const std::shared_ptr<Pawn>& instigator, const std::shared_ptr<Craft::Actor>& causer);

private:
    Facing _facing = Facing::Up;

    int _maxHp = 0;
    int _hp = 0;
    
    float _moveSpeed = 20.f;    // 초당 셀 20칸
    float _moveRemainder = 0.f;

    Craft::Vector2 _knockbackDirection = Craft::Vector2::Zero;
    int _remainKnockbackSteps = 0;
    float _knockbackRemainder = 0.f;

    Timer _invincibleTimer;
};

