#pragma once
#include <Actor/Actor.h>

enum class Facing
{
    Up = 0,
    Right,
    Down,
    Left,
    NONE
};

class Pawn : public Craft::Actor
{
    TYPE_DECLARATIONS(Pawn, Craft::Actor)

public:
    Pawn(Craft::Vector2 position, int maxHp);
    ~Pawn() override;

    // 내가 피해를 입었을 때 호출하는 API
    virtual void TakeDamage(int damageAmount, const std::shared_ptr<Pawn>& damageInstigator);
    inline int GetHp() const { return _hp; }
    inline bool IsDead() const { return _hp <= 0; }

    inline Facing GetFacing() const { return _facing; }
    void SetFacing(Facing facing) { _facing = facing; }

protected:
    virtual void OnDeath(const std::shared_ptr<Pawn>& damageInstigator);

private:
    Facing _facing = Facing::Up;
    int _maxHp = 0;
    int _hp = 0;
};

