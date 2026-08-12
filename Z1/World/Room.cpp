#include "pch.h"
#include "Room.h"

using namespace Craft;

bool RoomData::OutOfBound(int x, int y) const
{
    return x < 0 || x >= Width || y < 0 || y >= Height;
}

TileId RoomData::GetTileId(int x, int y) const
{
    if (OutOfBound(x, y)) return InvalidTileId;
    return tileIds[Index(x, y)];
}
