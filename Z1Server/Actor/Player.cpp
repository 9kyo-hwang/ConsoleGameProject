#include "pch.h"
#include "Player.h"

Player::Player(Vector2Int spawnPosition)
    : Actor(Z1::Protocol::ActorKind::Player)
{
    info.hp = 20;
    SetPosition(spawnPosition.x, spawnPosition.y);
}

std::int32_t Player::TakeDamage(std::int32_t amount)
{
    if (amount <= 0 || IsDead()) return 0;

    std::int32_t actualDamage = std::min<std::int32_t>(info.hp, amount);
    info.hp -= actualDamage;
    
    if (IsDead())
    {
        latestInput.moveDirection = Z1::Protocol::MoveDirection::None;
        info.flags |= Z1::Protocol::PlayerStateDead;
    }

    return actualDamage;
}
