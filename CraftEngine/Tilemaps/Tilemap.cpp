#include "pch.h"
#include "Tilemap.h"
#include <cassert>

namespace Craft
{
    Tilemap::Tilemap(Vector2 size, Vector2 cellSize)
        : _size(size), _cellSize(cellSize)
    {
        assert(_size.x > 0 && _size.y > 0);
        assert(_cellSize.x > 0 && _cellSize.y > 0);

        _tiles.resize((size_t)size.x * size.y);
    }

    bool Tilemap::IsInBounds(Vector2 cell) const
    {
        return 0 <= cell.x && cell.x < _size.x && 0 <= cell.y && cell.y < _size.y;
    }

    const Tile* Tilemap::GetTile(Vector2 cell) const
    {
        if (!IsInBounds(cell)) return nullptr;
        return &_tiles[GetIndex(cell)];
    }

    bool Tilemap::SetTile(Vector2 cell, Tile tile)
    {
        if (!IsInBounds(cell) || !tile.sprite || tile.sprite->GetSize() != _cellSize)
        {
            return false;
        }

        _tiles[GetIndex(cell)] = std::move(tile);
        return true;
    }

    bool Tilemap::CanPlaceBox(const Box2D& box) const
    {
        if (!box.IsValid())
        {
            return false;
        }

        const Vector2 mapWorldSize = _size * _cellSize;
        if (box.Left() < 0 || box.Top() < 0 || box.Right() >= mapWorldSize.x || box.Bottom() >= mapWorldSize.y)
        {
            return false;
        }

        const int minTileX = box.Left()     / _cellSize.x;
        const int minTileY = box.Top()      / _cellSize.y;
        const int maxTileX = box.Right()    / _cellSize.x;
        const int maxTileY = box.Bottom()   / _cellSize.y;

        for (int tileY = minTileY; tileY <= maxTileY; ++tileY)
        {
            for (int tileX = minTileX; tileX <= maxTileX; ++tileX)
            {
                if (GetTile(Vector2(tileX, tileY))->blocked)
                {
                    return false;
                }
            }
        }

        return true;
    }

    std::shared_ptr<const Sprite> Tilemap::BuildSprite(Vector2 origin, Vector2 count) const
    {
        const Vector2 spriteSize = count * _cellSize;
        std::vector<SpriteCell> cells(spriteSize.x * spriteSize.y);

        for (int tileY = 0; tileY < count.y; ++tileY)
        {
            for (int tileX = 0; tileX < count.x; ++tileX)
            {
                const Tile* tile = GetTile(origin + Vector2(tileX, tileY));
                if (!tile || !tile->sprite)
                {
                    continue;
                }

                const Vector2 targetOrigin = Vector2(tileX, tileY) * _cellSize;
                for (int cellY = 0; cellY < _cellSize.y; ++cellY)
                {
                    for (int cellX = 0; cellX < _cellSize.x; ++cellX)
                    {
                        const int targetIndex = (targetOrigin.y + cellY) * spriteSize.x + (targetOrigin.x + cellX);
                        cells[targetIndex] = tile->sprite->GetCell(cellX, cellY);
                    }
                }
            }
        }

        return std::make_shared<const Sprite>(spriteSize, std::move(cells));
    }

    size_t Tilemap::GetIndex(Vector2 cell) const
    {
        assert(IsInBounds(cell));
        return (size_t)cell.y * _size.x + cell.x;
    }
}