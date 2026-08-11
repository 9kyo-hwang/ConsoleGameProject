#include "pch.h"
#include "Room.h"

using namespace Craft;

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

bool RoomDefinition::CanOccupyWorldRect(const Vector2& localWorldPosition, const Vector2& worldSize, const Vector2& tileWorldSize) const
{
    // localWorldPosition: RoomOrigin 뺀 Room 내부 월드 좌표

    /*
    * 월드 사각형의 LTRB 계산
    - RB는 position + size - Vector2::One
    걸치는 타일 범위 계산
    - 범위가 Room 밖이면 false
    - 기존 CanOccupyTiles로 walkable 여부 계산
    */

    const int leftTile = localWorldPosition.x / tileWorldSize.x;
    const int topTile = localWorldPosition.y / tileWorldSize.y;

    const int rightWorld = localWorldPosition.x + worldSize.x - 1;
    const int bottomWorld = localWorldPosition.y + worldSize.y - 1;

    const int rightTile = rightWorld / tileWorldSize.x;
    const int bottomTile = bottomWorld / tileWorldSize.y;

    return CanOccupyTiles(leftTile, topTile, rightTile, bottomTile);
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
