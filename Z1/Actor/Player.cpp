#include "pch.h"
#include "Player.h"

#include <Component/BoxComponent.h>
#include <Component/SpriteRendererComponent.h>
#include <Actor/SwordAttack.h>

using namespace Craft;

Player::Player(Craft::Vector2 position, int maxHp)
    : Super(position, maxHp)
{
    // 렌더러에서 스케일에 비례하게 박스 크기를 늘리지 않도록 변경되어
    // 명시적으로 박스 크기를 렌더 스케일로 지정
    auto sprite = CreateSprite();
    _box = AddComponent<BoxComponent>(sprite->GetSize(), Vector2::Zero);
    _renderer = AddComponent<SpriteRendererComponent>(sprite, 10);
}

int Player::ConsumeMoveSteps(float deltaTime)
{
    _moveRemainder += _moveSpeed * deltaTime;

    const int steps = (int)_moveRemainder;
    _moveRemainder -= steps;

    return steps;
}

void Player::ClearMoveRemainder()
{
    _moveRemainder = 0.f;
}

// 이동 판정은 Room, 기본 이동 책임만 수행
void Player::MoveBy(const Craft::Vector2& delta)
{
    SetPosition(GetPosition() + delta);
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
