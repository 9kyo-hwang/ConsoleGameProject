#include "pch.h"
#include "Tektite.h"

#include <array>
#include <Math/MathUtility.h>
#include <Render/Sprite.h>

using namespace Craft;

namespace
{
    float HopCooldown(EnemyVariant variant)
    {
        return variant == EnemyVariant::Red ? 0.55f : 1.1f;
    }

    float HopDuration(EnemyVariant variant)
    {
        return variant == EnemyVariant::Red ? 0.45f : 0.3f;
    }

    std::shared_ptr<const Sprite> CreateTektiteSprite(Color bodyColor)
    {
        const std::array<std::string, 5> art
        {
            " \\ @@ / ",
            "  \\@@/  ",
            "/##@@##\\",
            "<#/||\\#>",
            "\\_/  \\_/"
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
                else if (glyph == '/' || glyph == '\\' || glyph == '|' ||
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

Tektite::Tektite(Vector2 position, EnemyVariant variant)
    : Super(position, GetVariantMaxHp(variant), 24.f, "T", GetVariantColor(variant))
    , _hopCooldown(HopCooldown(variant))
    , _hopDuration(HopDuration(variant))
{
    SetAppearance(CreateTektiteSprite(GetVariantColor(variant)));
}

void Tektite::Think(float deltaTime, const Player& player)
{
    if (_isHopping)
    {
        _hopDuration.Tick(deltaTime);
        if (_hopDuration.TimeOver())
        {
            _isHopping = false;
            _hopCooldown.Reset();
            desiredMove = Vector2::Zero;
            return;
        }

        desiredMove = _hopDirection;
        return;
    }

    _hopCooldown.Tick(deltaTime);
    if (!_hopCooldown.TimeOver())
    {
        desiredMove = Vector2::Zero;
        return;
    }

    _hopCooldown.Reset();
    _hopDuration.Reset();
    _hopDirection = GetHopDirection(player);
    _isHopping = true;
    desiredMove = _hopDirection;
}

void Tektite::OnMoveBlocked()
{
    _isHopping = false;
    _hopCooldown.Reset();
    desiredMove = Vector2::Zero;
}

Vector2 Tektite::GetHopDirection(const Player& player) const
{
    const Vector2 distance =
        player.GetWorldPosition() - GetWorldPosition();

    const int horizontal = distance.x >= 0 ? 1 : -1;
    const int vertical = distance.y >= 0 ? 1 : -1;
    const int randomAxis =
        FMath::RandRange(0, 1) == 0 ? 1 : -1;

    if (std::abs(distance.x) >= std::abs(distance.y))
    {
        return Vector2(horizontal, randomAxis);
    }

    return Vector2(randomAxis, vertical);
}
