#pragma once

#include <Math/Box2D.h>

template <typename IsTileWalkable>
bool CanPlaceBoxOnMap(
    const Craft::Box2D& box,
    const Craft::Vector2& tileCellSize,
    const Craft::Vector2& mapTileCount,
    IsTileWalkable&& isTileWalkable)
{
    if (!box.IsValid() ||
        tileCellSize.x <= 0 ||
        tileCellSize.y <= 0 ||
        mapTileCount.x <= 0 ||
        mapTileCount.y <= 0)
    {
        return false;
    }

    const int mapCellWidth = mapTileCount.x * tileCellSize.x;
    const int mapCellHeight = mapTileCount.y * tileCellSize.y;

    if (box.Left() < 0 ||
        box.Top() < 0 ||
        box.Right() >= mapCellWidth ||
        box.Bottom() >= mapCellHeight)
    {
        return false;
    }

    const int minTileX = box.Left() / tileCellSize.x;
    const int minTileY = box.Top() / tileCellSize.y;
    const int maxTileX = box.Right() / tileCellSize.x;
    const int maxTileY = box.Bottom() / tileCellSize.y;

    for (int tileY = minTileY; tileY <= maxTileY; ++tileY)
    {
        for (int tileX = minTileX; tileX <= maxTileX; ++tileX)
        {
            if (!isTileWalkable(tileX, tileY))
            {
                return false;
            }
        }
    }

    return true;
}
