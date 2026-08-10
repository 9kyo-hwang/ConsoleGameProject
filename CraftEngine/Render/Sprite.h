#pragma once
#include <Core/Core.h>
#include <Math/Color.h>
#include <Math/Vector2.h>
#include <vector>

namespace Craft
{
    // Pixel 한 칸
    struct SpriteCell
    {
        char glyph = ' ';
        WORD attributes = static_cast<WORD>(Color::White);
        bool transparent = true;
    };

    class CRAFT_API Sprite
    {
    public:
        Sprite(Vector2 size, std::vector<SpriteCell> cells);

        Vector2 GetSize() const;
        const SpriteCell& GetCell(int x, int y) const;

    private:
        Vector2 _size;
        std::vector<SpriteCell> _cells;
    };
}
