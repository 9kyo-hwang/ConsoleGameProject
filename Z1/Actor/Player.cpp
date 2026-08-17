#include "pch.h"
#include "Player.h"

#include <Component/BoxComponent.h>
#include <Component/SpriteRendererComponent.h>
#include <Actor/SwordAttack.h>
#include <Core/Input.h>
#include <Actor/Projectile.h>

using namespace Craft;

Player::Player(Craft::Vector2 position, int maxHp)
    : Super(position, maxHp, 30.f)
{
    // 렌더러에서 스케일에 비례하게 박스 크기를 늘리지 않도록 변경되어
    // 명시적으로 박스 크기를 렌더 스케일로 지정
    auto sprite = CreateSprite();
    _box = AddComponent<BoxComponent>(sprite->GetSize(), Vector2::Zero);
    _renderer = AddComponent<SpriteRendererComponent>(sprite, 10);
}

void Player::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _moveInput = Vector2::Zero; // 이동 방향을 나타내는 벡터 변수는 배 프레임 reset

    if (Input::Get().GetKey(VK_UP))
    {
        SetFacing(Facing::Up);
        _moveInput = Vector2::Up;
    }
    else if (Input::Get().GetKey(VK_DOWN))
    {
        SetFacing(Facing::Down);
        _moveInput = Vector2::Up * -1;
    }
    else if (Input::Get().GetKey(VK_LEFT))
    {
        SetFacing(Facing::Left);
        _moveInput = Vector2::Right * -1;
    }
    else if (Input::Get().GetKey(VK_RIGHT))
    {
        SetFacing(Facing::Right);
        _moveInput = Vector2::Right;
    }
}

bool Player::Shieldable(const Projectile& projectile) const
{
    if (IsAttacking())
    {
        return false;
    }

    if (!projectile.Shieldable())
    {
        return false;
    }

    const Vector2 shieldDirection =
        projectile.Direction() * -1;

    if (shieldDirection.x != 0 &&
        std::abs(shieldDirection.x) >= std::abs(shieldDirection.y))
    {
        return GetFacingDirection() ==
            Vector2(shieldDirection.x > 0 ? 1 : -1, 0);
    }

    if (shieldDirection.y != 0)
    {
        return GetFacingDirection() ==
            Vector2(0, shieldDirection.y > 0 ? 1 : -1);
    }

    return false;
}

bool Player::IsAttacking() const
{
    if (auto attack = _activeAttack.lock())
    {
        return attack->IsActive();
    }

    return false;
}

void Player::CancelAttack()
{
    if (auto attack = _activeAttack.lock())
    {
        attack->Destroy();
    }

    _activeAttack.reset();
}

void Player::OnDeath(const std::shared_ptr<Pawn>& damageInstigator)
{
    CancelAttack();
    // TODO: 사망 연출(?) -> 게임 오버 표시 -> Restart or Quit 선택 레벨
}

std::shared_ptr<const Craft::Sprite> Player::CreateSprite()
{
    std::vector<SpriteCell> cells
    {
        SpriteCell('v', (WORD)Color::White, false),
    };

    return std::make_shared<const Sprite>(Vector2::One, std::move(cells));
}
