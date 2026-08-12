#include "pch.h"
#include "Tile.h"

bool TileSpriteCatalog::TryGet(TileId id, std::shared_ptr<const Craft::Sprite>& sprite) const
{
    sprite.reset();
    if (_tiles.contains(id))
    {
        sprite = _tiles.at(id);
        return true;
    }

    return false;
}

void TileSpriteCatalog::Add(TileId id, std::shared_ptr<const Craft::Sprite> sprite)
{
    _tiles.insert_or_assign(id, sprite);
}
