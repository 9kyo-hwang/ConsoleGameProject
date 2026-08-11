#include "pch.h"
#include "Tile.h"

bool TileCatalog::Find(TileId id, TileDefinition& outTile) const
{
    if (_tiles.contains(id))
    {
        outTile = _tiles.at(id);
        return true;
    }

    return false;
}

void TileCatalog::Register(TileDefinition tile)
{
    _tiles.insert_or_assign(tile.id, tile);
}
