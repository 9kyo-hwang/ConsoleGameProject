#include "pch.h"
#include "CaveMap.h"
#include <World/MapGeometry.h>
#include <fstream>

using namespace Craft;

CaveMap::CaveMap()
    : _tilemap(Vector2(Width, Height), TileCellSize)
{
    InitializeTileSprites();
}

bool CaveMap::Load(const FilePath& path, std::string& errorMessage)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    std::vector<char> tiles(Width * Height);

    std::string line;
    for (int row = 0; row < Height; ++row)
    {
        if (!std::getline(file, line))
        {
            errorMessage = "CaveMap has fewer than 11 rows.";
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
            if (symbol != '#' && symbol != '.' && symbol != 'E' && symbol != 'S')
            {
                errorMessage = "CaveMap contains an invalid tile data.";
                return false;
            }

            tiles[row * Width + col] = symbol;
        }
    }

    if (std::getline(file, line))
    {
        errorMessage = "CaveMap has more than 11 rows.";
        return false;
    }

    Tilemap tilemap(Vector2(Width, Height), TileCellSize);

    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            const size_t index = y * Width + x;
            const char tile = tiles[index];
            const bool blocked = tile == '#';

            const auto found = _tileSprites.find(tile);
            if (found == _tileSprites.end())
            {
                // 음, fallback으로 '?' 처리 해줘야 하나?
                errorMessage = "_tileSprites에 없는 타일 종류";
                return false;
            }

            const bool set = tilemap.SetTile(Vector2(x, y), Tile(found->second, blocked));
            assert(set);
        }
    }

    _loaded = true;
    _tiles = std::move(tiles);
    _tilemap = std::move(tilemap);

    return true;
}

std::shared_ptr<const Craft::Sprite> CaveMap::BuildRoomSprite(Craft::Vector2 roomOrigin, Craft::Vector2 roomSize) const
{
    return _tilemap.BuildSprite(roomOrigin, roomSize);
}

bool CaveMap::CanPlaceBox(const Craft::Box2D& box) const
{
    return _tilemap.CanPlaceBox(box);
}

char CaveMap::GetTile(int x, int y) const
{
    assert(_tilemap.IsInBounds(Vector2(x, y)));
    return _tiles[y * _tilemap.GetSize().x + x];
}

bool CaveMap::IsWalkable(int x, int y) const
{
    if (const Tile* tile = _tilemap.GetTile(Vector2(x, y)))
    {
        return !tile->blocked;
    }

    return false;
}

void CaveMap::InitializeTileSprites()
{
    const auto makeTileSprite = [](char glyph, Color color)
        {
            return Sprite::Create(TileCellSize, glyph, color);
        };

    _tileSprites.emplace('#', makeTileSprite('#', Color::DarkRed));
    _tileSprites.emplace('.', makeTileSprite(' ', Color::Black));
    _tileSprites.emplace('E', makeTileSprite(' ', Color::Black));
    _tileSprites.emplace('S', makeTileSprite(' ', Color::Black));
}
