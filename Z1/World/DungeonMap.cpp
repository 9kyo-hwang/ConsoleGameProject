#include "pch.h"
#include "DungeonMap.h"
#include <fstream>

using namespace Craft;

bool DungeonMap::Load(const FilePath& path, std::string& errorMessage)
{
    if (_loaded) return false;

    std::ifstream file(path);
    if (!file.is_open())
    {
        errorMessage = "DungeonMap file could not be opened.";
        return false;
    }

    std::string line;
    for (int row = 0; row < Height; ++row)
    {
        if (!std::getline(file, line))
        {
            errorMessage = "DungeonMap has fewer than 11 rows.";
            return false;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.size() != Width)
        {
            return false;
        }

        for (int col = 0; col < Width; ++col)
        {
            char symbol = line[col];
            if (symbol != '#' && symbol != '.' && symbol != 'x' && symbol != 'B' && symbol != 'H' && symbol != 'T')
            {
                errorMessage = "DungeonMap contains an invalid tile data.";
                return false;
            }

            _grid[row][col] = symbol;
        }
    }

    if (std::getline(file, line))
    {
        errorMessage = "TileMap has more than 88 rows.";
        return false;
    }

    return true;
}

bool DungeonMap::CanOccupyWorldRect(const Vector2& boxColliderPosition, const Vector2& boxColliderSize, const Vector2& tileSize) const
{
    if (boxColliderSize.x <= 0 || boxColliderSize.y <= 0)
    {
        return false;
    }

    const int left = boxColliderPosition.x;
    const int top = boxColliderPosition.y;
    const int right = left + boxColliderSize.x - 1;
    const int bottom = top + boxColliderSize.y - 1;

    if (tileSize.x <= 0 || tileSize.y <= 0)
    {
        return false;
    }

    // 셀 개수를 반영한 맵 경계 파악: 셀 개수 x 셀 하나 당 도트 개수(타일 크기)
    const int mapWidth = Width * tileSize.x;
    const int mapHeight = Height * tileSize.y;

    if (left < 0 || top < 0 || right >= mapWidth || bottom >= mapHeight)
    {
        return false;
    }

    // 타일의 논리적 좌표(도트 개수만큼 나누기)
    const int minTileX = left / tileSize.x;
    const int minTileY = top / tileSize.y;
    const int maxTileX = right / tileSize.x;
    const int maxTileY = bottom / tileSize.y;

    // 도트가 속하는 타일 중 하나라도 이동 불가라면
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

char DungeonMap::GetTile(int x, int y) const
{
    assert(!(x < 0 || x >= Width || y < 0 || y >= Height));
    return _grid[y][x];
}

bool DungeonMap::IsWalkable(int x, int y) const
{
    if (x < 0 || x >= Width || y < 0 || y >= Height) return false;
    const char tile = _grid[y][x];

    return tile != '#' && tile != 'x';
}
