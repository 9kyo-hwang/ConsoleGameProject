#pragma once
#include <Core/Core.h>
#include <Math/Color.h>
#include <Math/Vector2.h>
#include <vector>
#include <string>

namespace Craft
{
    // Pixel 한 칸
    struct CRAFT_API SpriteCell
    {
        char glyph = ' ';
        WORD attributes = static_cast<WORD>(Color::White);
        bool transparent = true;
    };

    // Pixel 뭉치(size.x x size.y)
    class CRAFT_API Sprite
    {
    public:
        Sprite(Vector2 size, std::vector<SpriteCell> cells);

        Vector2 GetSize() const;
        const SpriteCell& GetCell(int x, int y) const;  // 스프라이트 내부 좌표에 해당하는 셀 반환

    public:
        static std::shared_ptr<const Sprite> Create(std::string text, Color color = Color::White);
        static std::shared_ptr<const Sprite> Create(Vector2 size, char glyph, Color color = Color::White);

    private:
        Vector2 _size;
        std::vector<SpriteCell> _cells;
    };
}
