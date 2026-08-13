#include "pch.h"
#include "Pawn.h"

using namespace Craft;

Pawn::Pawn(Vector2 position, int maxHp, float moveSpeed)
    : Super(position)
    , _maxHp(maxHp)
    , _hp(maxHp)
    , _moveSpeed(moveSpeed)
{
}

Pawn::~Pawn()
{
}

void Pawn::TakeDamage(int damageAmount, const std::shared_ptr<Pawn>& damageInstigator)
{
    // TODO: 현재 Facing 방향으로 밀림 처리

    _hp -= damageAmount;
    if (_hp <= 0)
    {
        _hp = 0;
        OnDeath(damageInstigator);
    }
}

int Pawn::ConsumeMoveSteps(float deltaTime)
{
    _moveRemainder += _moveSpeed * deltaTime;

    const int steps = (int)_moveRemainder;
    _moveRemainder -= steps;

    return steps;
}

void Pawn::ClearMoveRemainder()
{
    _moveRemainder = 0.f;
}

void Pawn::MoveBy(const Craft::Vector2& delta)
{
    SetPosition(GetPosition() + delta);
}

void Pawn::OnDeath(const std::shared_ptr<Pawn>& damageInstigator)
{

}
