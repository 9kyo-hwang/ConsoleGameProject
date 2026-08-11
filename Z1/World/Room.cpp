#include "pch.h"
#include "Room.h"

bool RoomDefinition::OutOfBound(int x, int y) const
{
    return x < 0 || x >= Width || y < 0 || y >= Height;
}

TileId RoomDefinition::GetTileId(int x, int y) const
{
    if (OutOfBound(x, y)) return InvalidTileId;
    return tileIds[Index(x, y)];
}

bool RoomDefinition::IsWalkable(int x, int y) const
{
    if (OutOfBound(x, y)) return false;
    return walkable[Index(x, y)];
}

bool RoomDefinition::CanOccupyTiles(int minTileX, int minTileY, int maxTileX, int maxTileY) const
{
    if (minTileX < 0 || minTileY < 0 || maxTileX >= Width || maxTileY >= Height)
    {
        return false;
    }

    for (int y = minTileY; y <= maxTileY; ++y)
    {
        for (int x = minTileX; x <= maxTileX; ++x)
        {
            if (!IsWalkable(x, y))
            {
                return false;
            }
        }
    }

    return true;
}
