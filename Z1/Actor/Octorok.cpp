#include "pch.h"
#include "Octorok.h"

#include <Math/MathUtility.h>
#include <array>
#include <Render/Sprite.h>

using namespace Craft;

namespace
{
    std::shared_ptr<const Sprite> CreateOctorokSprite(Color bodyColor)
    {
        const std::array<std::string, 5> art
        {
            "   /\\   ",
            "/######\\",
            "<##@@##>",
            "\\######/",
            " /####\\ "
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
                else if (glyph == '/' || glyph == '\\' ||
                         glyph == '<' || glyph == '>')
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

Octorok::Octorok(Craft::Vector2 position, EnemyVariant variant)
    : Super(position, GetVariantMaxHp(variant), 8.f, "O", GetVariantColor(variant))
    , _rotateTimer(0.75f)
    , _attackTimer(1.5f)
{
    SetAppearance(CreateOctorokSprite(GetVariantColor(variant)));

    Rotate();
}

void Octorok::Think(float deltaTime, const Player&)
{
    _rotateTimer.Tick(deltaTime);
    _attackTimer.Tick(deltaTime);

    if (_rotateTimer.TimeOver())
    {
        _rotateTimer.Reset();
        Rotate();
    }

    desiredMove = _direction;

    if (_attackTimer.TimeOver())
    {
        _attackTimer.Reset();
        RequestRockAttack();
    }
}

Vector2 Octorok::GetRandomDirection() const
{
    switch (FMath::RandRange(0, 3))
    {
    case 0: return Vector2::Up;
    case 1: return Vector2::Right;
    case 2: return Vector2::Up * -1;
    case 3: return Vector2::Right * -1;
    }

    return Vector2::Right;
}

void Octorok::Rotate()
{
    _direction = GetRandomDirection();
}

void Octorok::RequestRockAttack()
{
    // 바라보는 방향으로 공격

    EnemyAttackRequest request;

    request.projectiles.emplace_back(ProjectileSpec
    {
        .type = ProjectileType::Rock,
        .faction = ProjectileFaction::Enemy,
        .damage = 1,
        .speed = 20.f,
        .lifetime = 2.f,
        .direction = _direction,
        .boxSize = Vector2::One,
        .image = "o",
        .color = Color::DarkYellow,
        .sortingOrder = 12
    });

    RequestAttack(request);
}
