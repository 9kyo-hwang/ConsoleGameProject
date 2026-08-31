#include "pch.h"
#include "Player.h"

#include <Component/BoxComponent.h>
#include <Component/SpriteRendererComponent.h>
#include <Actor/SwordAttack.h>
#include <Core/Input.h>
#include <Actor/Projectile.h>
#include <array>
#include <Engine/Engine.h>
#include <Render/Sprite.h>

using namespace Craft;

Player::Player(Vector2 position, int maxHp)
    : Super(position, maxHp, 20.f)
{
    // 렌더러에서 스케일에 비례하게 박스 크기를 늘리지 않도록 변경되어
    // 명시적으로 박스 크기를 렌더 스케일로 지정
    auto sprite = CreateSprite();
    _renderer = AddComponent<SpriteRendererComponent>(sprite, 10);
    _box = AddComponent<BoxComponent>(sprite->GetSize(), Vector2::Zero);
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

int Player::TakeDamage(
    int amount,
    const std::shared_ptr<Pawn>& instigator,
    const std::shared_ptr<Craft::Actor>& causer
)
{
    const int actualDamage =
        Super::TakeDamage(amount, instigator, causer);

    if (actualDamage > 0 && !IsDead())
    {
        Engine::Get().PlayOneShot("Z1/LOZ_Link_Hurt.wav");
    }

    return actualDamage;
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
    Engine::Get().PlayOneShot("Z1/LOZ_Link_Die.wav");
}

std::shared_ptr<const Craft::Sprite> Player::CreateSprite()
{
    const std::array<std::string, 5> art
    {
        "   /\\   ",
        "  /@@\\  ",
        "<######>",
        " /####\\ ",
        "   ||   "
    };

    std::vector<SpriteCell> cells;
    cells.reserve(40);

    for (const std::string& row : art)
    {
        for (const char glyph : row)
        {
            Color color = Color::Green;
            if (glyph == '@')
            {
                color = Color::White;
            }
            else if (glyph == '/' || glyph == '\\' ||
                     glyph == '<' || glyph == '>')
            {
                color = Color::DarkGreen;
            }
            else if (glyph == '|')
            {
                color = Color::DarkYellow;
            }

            cells.emplace_back(SpriteCell
            {
                glyph,
                static_cast<WORD>(color),
                glyph == ' '
            });
        }
    }

    return std::make_shared<const Sprite>(
        Vector2(8, 5),
        std::move(cells)
    );
}
