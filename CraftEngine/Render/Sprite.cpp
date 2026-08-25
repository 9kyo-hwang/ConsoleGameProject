#include "pch.h"
#include "Sprite.h"

namespace Craft
{
    Sprite::Sprite(Vector2 size, std::vector<SpriteCell> cells)
        : _size(size)
        , _cells(std::move(cells))
    {
        assert(_size.x >= 0 && _size.y >= 0);
        assert(_cells.size() == static_cast<size_t>(_size.x * _size.y));
    }

    Vector2 Sprite::GetSize() const
    {
        return _size;
    }

    const SpriteCell& Sprite::GetCell(int x, int y) const
    {
        assert(x >= 0 && x < _size.x && y >= 0 && y < _size.y);
        return _cells[y * _size.x + x];
    }

    /*
    * 문자열 렌더링을 대체하는 N x 1 스프라이트 생성기
    * 모든 셀 transparent = false, 동일 색상, 줄바꿈 처리 X
    * 빈 문자열은 (0, 1) 크기 빈 스프라이트
    */
    std::shared_ptr<const Sprite> Sprite::Create(std::string text, Color color)
    {
        const Vector2 size((int)text.size(), 1);

        std::vector<SpriteCell> cells;
        cells.reserve(text.size());

        for (char glyph : text)
        {
            cells.emplace_back(SpriteCell(glyph, (WORD)color, false));
        }

        return std::make_shared<const Sprite>(size, std::move(cells));
    }

    /*
    * size만큼 glyph로 채우기
    * 공백도 불투명 -> Create(Vector2(10, 5), ' ', Color::Black)은 검은 배경
    * 음수 크기는 assert
    */
    std::shared_ptr<const Sprite> Sprite::Create(Vector2 size, char glyph, Color color)
    {
        if (size.x < 0 || size.y < 0)
        {
            assert(false && "Sprite::Filled size must not be negative");
            return nullptr;
        }

        const size_t numCells = (size_t)size.x * (size_t)size.y;
        const SpriteCell cell(glyph, (WORD)color, false);
        
        std::vector<SpriteCell> cells(numCells, cell);

        return std::make_shared<const Sprite>(size, std::move(cells));
    }
}
