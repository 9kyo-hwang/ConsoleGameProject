#include "pch.h"
#include "SwordSprite.h"

#include <Render/Sprite.h>

#include <string>
#include <utility>
#include <vector>

using namespace Craft;

std::shared_ptr<const Sprite> CreateSwordSprite(const Vector2& direction)
{
    const std::vector<std::string> art =
        direction.x > 0
        ? std::vector<std::string>
        {
            "     >    ",
            " =======> ",
            "     >    "
        }
        : direction.x < 0
        ? std::vector<std::string>
        {
            "    <     ",
            " <======= ",
            "    <     "
        }
        : direction.y < 0
        ? std::vector<std::string>
        {
            "   ^  ",
            "   ^  ",
            "  ^^^ ",
            "   |  ",
            "   |  "
        }
        : std::vector<std::string>
        {
            "   |  ",
            "   |  ",
            "  vvv ",
            "   v  ",
            "   v  "
        };

    const Vector2 spriteSize(
        static_cast<int>(art[0].size()),
        static_cast<int>(art.size())
    );

    std::vector<SpriteCell> cells;
    cells.reserve(spriteSize.x * spriteSize.y);

    for (const std::string& row : art)
    {
        for (const char glyph : row)
        {
            cells.emplace_back(SpriteCell
            {
                glyph,
                static_cast<WORD>(Color::White),
                glyph == ' '
            });
        }
    }

    return std::make_shared<const Sprite>(
        spriteSize,
        std::move(cells)
    );
}
