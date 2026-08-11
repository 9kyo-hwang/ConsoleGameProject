#include "pch.h"
#include "Player.h"

#include <Component/BoxComponent.h>
#include <Component/SpriteRendererComponent.h>

using namespace Craft;

Player::Player(Craft::Vector2 position, Craft::Vector2 renderScale)
    : Super(position)
{
    _box = AddComponent<BoxComponent>(Vector2::One, Vector2::Zero);
    _renderer = AddComponent<SpriteRendererComponent>(CreateSprite(), 10);
    _renderer->SetCellScale(renderScale);
}

// 이동 판정은 Room, 기본 이동 책임만 수행
void Player::MoveBy(const Craft::Vector2& delta)
{
    SetPosition(GetPosition() + delta);
}

std::shared_ptr<const Craft::Sprite> Player::CreateSprite()
{
    std::vector<SpriteCell> cells
    {
        SpriteCell('P', (WORD)Color::White, false),
    };

    return std::make_shared<const Sprite>(Vector2::One, std::move(cells));
}
