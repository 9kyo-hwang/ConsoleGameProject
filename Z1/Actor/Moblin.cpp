#include "pch.h"
#include "Moblin.h"

#include <array>
#include <Render/Sprite.h>

using namespace Craft;

namespace
{
    std::shared_ptr<const Sprite> CreateMoblinSprite(Color bodyColor)
    {
        const std::array<std::string, 5> art
        {
            " /====\\ ",
            "/|o@@o|\\",
            " |####| ",
            " /|##|\\ ",
            "/_/  \\_\\"
        };

        std::vector<SpriteCell> cells;
        cells.reserve(40);

        for (const std::string& row : art)
        {
            for (const char glyph : row)
            {
                Color color = bodyColor;
                if (glyph == '@')
                {
                    color = Color::Yellow;
                }
                else if (glyph == '/' || glyph == '\\' || glyph == '=')
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
}

Moblin::Moblin(Vector2 position, EnemyVariant variant)
    : Super(position, GetVariantMaxHp(variant), 6.f, "M", GetVariantColor(variant))
    , _directionTimer(0.8f)
    , _attackTimer(2.2f)
{
    SetAppearance(CreateMoblinSprite(GetVariantColor(variant)));
}

void Moblin::Think(float deltaTime, const Player& player)
{
    _directionTimer.Tick(deltaTime);
    _attackTimer.Tick(deltaTime);

    if (_directionTimer.TimeOver())
    {
        _directionTimer.Reset();
        _direction = GetChaseDelta(player.GetWorldPosition());
    }

    desiredMove = _direction;

    if (_attackTimer.TimeOver())
    {
        _attackTimer.Reset();
        _direction = GetChaseDelta(player.GetWorldPosition());
        RequestSpearAttack();
    }
}

void Moblin::OnMoveBlocked()
{
    _direction = _direction * -1;
}

void Moblin::RequestSpearAttack()
{
    const bool isHorizontal = _direction.x != 0;

    EnemyAttackRequest request;
    request.projectiles.emplace_back(ProjectileSpec
    {
        .type = ProjectileType::Spear,
        .faction = ProjectileFaction::Enemy,
        .damage = 1,
        .speed = 24.f,
        .lifetime = 2.f,
        .direction = _direction,
        .boxSize = Vector2::One,
        .image = isHorizontal ? "-" : "|",
        .color = Color::DarkYellow,
        .sortingOrder = 12
    });

    RequestAttack(request);
}
