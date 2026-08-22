#include "pch.h"
#include "CombatEffect.h"

#include <array>
#include <Component/SpriteRendererComponent.h>
#include <Render/Sprite.h>

using namespace Craft;

namespace
{
    std::shared_ptr<const Sprite> MakeSprite(
        const std::array<std::string, 3>& art,
        Color color
    )
    {
        std::vector<SpriteCell> cells;
        cells.reserve(9);

        for (const std::string& row : art)
        {
            for (const char glyph : row)
            {
                cells.emplace_back(SpriteCell
                {
                    glyph,
                    static_cast<WORD>(color),
                    glyph == ' '
                });
            }
        }

        return std::make_shared<const Sprite>(
            Vector2(3, 3),
            std::move(cells)
        );
    }
}

std::shared_ptr<const Craft::Sprite> CreateCombatEffectSprite(
    CombatEffectType type,
    Craft::Color color
)
{
    switch (type)
    {
    case CombatEffectType::Hit:
        return MakeSprite(
            { " + ", "+*+", " + " },
            color
        );

    case CombatEffectType::Death:
        return MakeSprite(
            { "\\|/", "-*-", "/|\\" },
            color
        );
    }

    return nullptr;
}

CombatEffect::CombatEffect(
    Vector2 position,
    std::shared_ptr<const Sprite> sprite,
    float duration,
    int sortingOrder
)
    : Super(position)
    , _timer(duration)
{
    AddComponent<SpriteRendererComponent>(
        std::move(sprite),
        sortingOrder
    );
}

void CombatEffect::Tick(float deltaTime)
{
    Super::Tick(deltaTime);

    _timer.Tick(deltaTime);
    if (_timer.TimeOver())
    {
        Destroy();
    }
}
