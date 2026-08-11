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
