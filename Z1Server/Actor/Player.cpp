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
        _latestInput.moveDirection = Z1::Protocol::MoveDirection::None;
        info.flags |= Z1::Protocol::PlayerStateDead;
    }

    return actualDamage;
}

void Player::UpdateInput(const Z1::Protocol::InputCommand& input) noexcept
{
    _lastInputSequence = input.sequence;
    _latestInput = input;

    if ((input.actionFlags & Z1::Protocol::InputActionAttack) != 0)
    {
        _attackRequested = true;
    }

    _latestInput.actionFlags = 0;
}

int Player::ConsumeMoveSteps(float deltaTime)
{
    // 서버에서는 고정된 델타 타임 TickInterval 50ms가 넘어옴
    _moveRemainder += deltaTime * _moveSpeed;
    int step = (int)_moveRemainder; // 소수점 떼고 칸 만큼 이동
    _moveRemainder -= step;
    return step;
}
