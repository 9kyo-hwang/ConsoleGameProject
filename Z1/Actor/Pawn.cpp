#include "pch.h"
#include "Pawn.h"

using namespace Craft;

namespace 
{
    constexpr int KnockbackDistance = 4;    // 밀려날 픽셀
    constexpr float KnockbackSpeed = 40.f;  // 초당 밀려날 속도  
    constexpr float InvincibleTime = 0.75f;  // 피격 무적 시간
    constexpr float BlinkInterval = 0.08f;  // 깜빡임 간격
}

Pawn::Pawn(Vector2 position, int maxHp, float moveSpeed)
    : Super(position)
    , _maxHp(maxHp)
    , _hp(maxHp)
    , _moveSpeed(moveSpeed)
    , _invincibleTimer(InvincibleTime)
{
    _invincibleTimer.Complete();
}

Pawn::~Pawn()
{
}

void Pawn::Tick(float deltaTime)
{
    Super::Tick(deltaTime);
    _invincibleTimer.Tick(deltaTime);
}

void Pawn::Draw()
{
    // TODO: 추후 SpriteRendererComponent에 SetVisible()을...?
    if (!_invincibleTimer.TimeOver())
    {
        // 일단 임시로 숨겨진 프레임에선 Draw를 호출 안하도록 수정
        const int phase = (int)(_invincibleTimer.ElapsedTime() / BlinkInterval);
        if (phase % 2 == 0)
        {
            return;
        }
    }

    Super::Draw();
}

int Pawn::TakeDamage(int amount, const std::shared_ptr<Pawn>& instigator, const std::shared_ptr<Craft::Actor>& causer)
{
    if (amount <= 0 || IsDead())
    {
        return 0;
    }

    if (!_invincibleTimer.TimeOver())
    {
        return 0;
    }

    int actualDamage = std::min<int>(amount, _hp);
    _hp -= actualDamage;

    if (IsDead())
    {
        OnDeath(instigator);
        return actualDamage;
    }

    _invincibleTimer.Reset();
    Knockback(instigator, causer);

    return actualDamage;
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

int Pawn::ConsumeKnockbackSteps(float deltaTime)
{
    if (!IsKnockback())
    {
        return 0;
    }

    _knockbackRemainder += KnockbackSpeed * deltaTime;
    int steps = std::min<int>((int)_knockbackRemainder, _remainKnockbackSteps);

    _knockbackRemainder -= steps;
    _remainKnockbackSteps -= steps;

    if (_remainKnockbackSteps == 0)
    {
        _knockbackRemainder = 0.f;
    }

    return steps;
}

void Pawn::StopKnockback()
{
    _knockbackDirection = Vector2::Zero;
    _remainKnockbackSteps = 0;
    _knockbackRemainder = 0.f;
}

Craft::Vector2 Pawn::GetFacingDirection() const
{
    switch (GetFacing())
    {
    case Facing::Up:    return Vector2::Up;
    case Facing::Right: return Vector2::Right;
    case Facing::Down:  return Vector2::Up * -1;
    case Facing::Left:  return Vector2::Right * -1;
    default: return Vector2::Zero;
    }
}

void Pawn::Knockback(const std::shared_ptr<Pawn>& instigator, const std::shared_ptr<Craft::Actor>& causer)
{
    Vector2 direction = Vector2::Zero;
    if (instigator && causer)
    {
        // 공격자 -> 무기 방향
        direction = causer->GetWorldPosition() - instigator->GetWorldPosition();
    }

    if (direction == Vector2::Zero && instigator)
    {
        // 피해자가 공격자로부터 떨어지는 방향 
        direction = GetWorldPosition() - causer->GetWorldPosition();
    }

    if (direction == Vector2::Zero)
    {
        return;
    }

    if (std::abs(direction.x) >= std::abs(direction.y))
    {
        _knockbackDirection = direction.x >= 0 ? Vector2::Right : Vector2::Right * -1;
    }
    else
    {
        _knockbackDirection = direction.y >= 0 ? Vector2::Up * -1: Vector2::Up;
    }

    _remainKnockbackSteps = KnockbackDistance;
    _knockbackRemainder = 0.f;

    // 이동 누적값이 넉백 직후 적용되지 않도록?
    ClearMoveRemainder();
}
