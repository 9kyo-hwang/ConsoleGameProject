#include "pch.h"
#include "Pawn.h"

using namespace Craft;

Pawn::Pawn(Vector2 position, int maxHp)
    : Super(position)
    , _maxHp(maxHp)
    , _hp(maxHp)
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

void Pawn::OnDeath(const std::shared_ptr<Pawn>& damageInstigator)
{

}
