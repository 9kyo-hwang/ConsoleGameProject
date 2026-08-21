#pragma once

#include <Util/BoxBounds.h>

template <typename IsTileWalkable>
bool CanPlaceBoxOnMap(
    const BoxBounds& boxBounds,
    const Craft::Vector2& tileCellSize,
    const Craft::Vector2& mapTileCount,
    IsTileWalkable&& isTileWalkable)
{
    if (!boxBounds.IsValid() ||
        tileCellSize.x <= 0 ||
        tileCellSize.y <= 0 ||
        mapTileCount.x <= 0 ||
        mapTileCount.y <= 0)
    {
        return false;
    }

    const int mapCellWidth = mapTileCount.x * tileCellSize.x;
    const int mapCellHeight = mapTileCount.y * tileCellSize.y;

    if (boxBounds.Left() < 0 ||
        boxBounds.Top() < 0 ||
        boxBounds.Right() >= mapCellWidth ||
        boxBounds.Bottom() >= mapCellHeight)
    {
        return false;
    }

    const int minTileX = boxBounds.Left() / tileCellSize.x;
    const int minTileY = boxBounds.Top() / tileCellSize.y;
    const int maxTileX = boxBounds.Right() / tileCellSize.x;
    const int maxTileY = boxBounds.Bottom() / tileCellSize.y;

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
