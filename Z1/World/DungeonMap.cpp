#include "pch.h"
#include "DungeonMap.h"
#include <Util/MapPlacement.h>
#include <World/MapGeometry.h>
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

            _tiles[row][col] = symbol;
        }
    }

    if (std::getline(file, line))
    {
        errorMessage = "TileMap has more than 88 rows.";
        return false;
    }

    _loaded = true;
    return true;
}

bool DungeonMap::CanPlaceBox(const BoxBounds& boxBounds) const
{
    return CanPlaceBoxOnMap(
        boxBounds,
        TileCellSize,
        Vector2(Width, Height),
        [this](int x, int y)
        {
            return IsWalkable(x, y);
        }
    );
}

char DungeonMap::GetTile(int x, int y) const
{
    assert(!(x < 0 || x >= Width || y < 0 || y >= Height));
    return _tiles[y][x];
}

bool DungeonMap::IsWalkable(int x, int y) const
{
    if (x < 0 || x >= Width || y < 0 || y >= Height) return false;
    const char tile = _tiles[y][x];

    return tile != '#' && tile != 'x';
}
