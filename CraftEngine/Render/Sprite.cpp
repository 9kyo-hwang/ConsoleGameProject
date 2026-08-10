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
}