#include "pch.h"
#include "Aquamentus.h"

#include <array>
#include <Component/BoxComponent.h>
#include <Component/SpriteRendererComponent.h>
#include <Render/Sprite.h>

using namespace Craft;

namespace
{
    std::shared_ptr<const Sprite> CreateAquamentusSprite()
    {
        const std::array<std::string, 5> art
        {
            "  ^  ",
            " /|\\ ",
            "<@#@>",
            "/###\\",
            " v v "
        };

        std::vector<SpriteCell> cells;
        cells.reserve(25);

        for (const std::string& row : art)
        {
            for (const char glyph : row)
            {
                const bool transparent = glyph == ' ';

                Color color = Color::Green;
                if (glyph == '^')
                {
                    color = Color::DarkYellow;
                }
                else if (glyph == '@')
                {
                    color = Color::Yellow;
                }

                cells.emplace_back(SpriteCell
                {
                    glyph,
                    static_cast<WORD>(color),
                    transparent
                });
            }
        }

        return std::make_shared<const Sprite>(
            Vector2(5, 5),
            std::move(cells)
        );
    }
}

Aquamentus::Aquamentus(Vector2 position)
    : Super(position, 6, 3.f, " ", Color::Green)
    , _attackTimer(2.f)
{
    if (const auto renderer = GetComponent<SpriteRendererComponent>())
    {
        renderer->SetSprite(CreateAquamentusSprite());
        renderer->SetSortingOrder(11);
    }

    if (const auto box = GetComponent<BoxComponent>())
    {
        box->SetSize(Vector2(5, 5));
    }
}

void Aquamentus::Think(float deltaTime, const Player& player)
{
    _attackTimer.Tick(deltaTime);

    if (_attackTimer.TimeOver())
    {
        _attackTimer.Reset();
        RequestFireballAttack(player);
    }

    desiredMove = _moveDirection;
}

void Aquamentus::OnMoveBlocked()
{
    _moveDirection = _moveDirection * -1;
}

void Aquamentus::RequestFireballAttack(const Player& player)
{
    const Vector2 forward =
        GetChaseDelta(player.GetWorldPosition());

    std::array<Vector2, 3> directions;

    if (forward.x != 0)
    {
        directions =
        {
            Vector2(forward.x, -1),
            Vector2(forward.x, 0),
            Vector2(forward.x, 1)
        };
    }
    else
    {
        directions =
        {
            Vector2(-1, forward.y),
            Vector2(0, forward.y),
            Vector2(1, forward.y)
        };
    }

    EnemyAttackRequest request;
    request.projectiles.reserve(directions.size());

    for (const Vector2& direction : directions)
    {
        request.projectiles.emplace_back(ProjectileSpec
        {
            .type = ProjectileType::Fireball,
            .faction = ProjectileFaction::Enemy,
            .damage = 1,
            .speed = 30.f,
            .lifetime = 4.f,
            .direction = direction,
            .boxSize = Vector2::One,
            .image = "*",
            .color = Color::Red,
            .sortingOrder = 12
        });
    }

    RequestAttack(request);
}
