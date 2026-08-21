#include "pch.h"
#include "Moblin.h"

#include <array>
#include <Component/BoxComponent.h>
#include <Component/SpriteRendererComponent.h>
#include <Render/Sprite.h>

using namespace Craft;

namespace
{
    int MaxHp(EnemyVariant variant)
    {
        switch (variant)
        {
        case EnemyVariant::Red: return 1;
        case EnemyVariant::Blue: return 2;
        }

        return 1;
    }

    Color GetColor(EnemyVariant variant)
    {
        switch (variant)
        {
        case EnemyVariant::Red: return Color::Red;
        case EnemyVariant::Blue: return Color::Blue;
        }

        return Color::Red;
    }

    std::shared_ptr<const Sprite> CreateMoblinSprite(Color bodyColor)
    {
        const std::array<std::string, 5> art
        {
            "  /====\\  ",
            " /|o@@o|\\ ",
            "  |####|  ",
            "  /|##|\\  ",
            " /_/  \\_\\ "
        };

        std::vector<SpriteCell> cells;
        cells.reserve(50);

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
            Vector2(10, 5),
            std::move(cells)
        );
    }
}

Moblin::Moblin(Vector2 position, EnemyVariant variant)
    : Super(position, MaxHp(variant), 6.f, "M", GetColor(variant))
    , _variant(variant)
    , _directionTimer(0.8f)
    , _attackTimer(2.2f)
{
    if (const auto renderer = GetComponent<SpriteRendererComponent>())
    {
        renderer->SetSprite(CreateMoblinSprite(GetColor(variant)));
        renderer->SetSortingOrder(11);
    }

    if (const auto box = GetComponent<BoxComponent>())
    {
        box->SetSize(Vector2(10, 5));
    }
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
