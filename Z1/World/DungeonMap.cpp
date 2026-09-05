#include "pch.h"
#include "DungeonMap.h"
#include <World/MapGeometry.h>
#include <fstream>

using namespace Craft;

DungeonMap::DungeonMap()
    : _tilemap(Vector2(Width, Height), TileCellSize)
{
    InitializeTileSprites();
}

bool DungeonMap::Load(const FilePath& path, std::string& errorMessage)
{
    if (_loaded) return false;

    std::ifstream file(path);
    if (!file.is_open())
    {
        errorMessage = "DungeonMap file could not be opened.";
        return false;
    }

    std::vector<char> tiles(Width * Height);

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

            tiles[row * Width + col] = symbol;
        }
    }

    if (std::getline(file, line))
    {
        errorMessage = "TileMap has more than 88 rows.";
        return false;
    }

    Tilemap tilemap(Vector2(Width, Height), TileCellSize);

    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            const size_t index = y * Width + x;
            const char tile = tiles[index];
            const bool blocked = tile == '#' || tile == 'x';

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

std::shared_ptr<const Craft::Sprite> DungeonMap::BuildRoomSprite(Craft::Vector2 roomOrigin, Craft::Vector2 roomSize) const
{
    return _tilemap.BuildSprite(roomOrigin, roomSize);
}

bool DungeonMap::CanPlaceBox(const Box2D& box) const
{
    return _tilemap.CanPlaceBox(box);
}

char DungeonMap::GetTile(int x, int y) const
{
    assert(_tilemap.IsInBounds(Vector2(x, y)));
    return _tiles[y * _tilemap.GetSize().x + x];
}

void DungeonMap::InitializeTileSprites()
{
    const auto makeTileSprite = [](char glyph, Color color)
        {
            return Sprite::Create(TileCellSize, glyph, color);
        };

    _tileSprites.emplace('.', makeTileSprite(' ', Color::Black));
    _tileSprites.emplace('#', makeTileSprite('#', Color::DarkRed));
    _tileSprites.emplace('x', makeTileSprite('x', Color::DarkGray));
    _tileSprites.emplace('B', makeTileSprite(' ', Color::Black));   // 보스도 아이템처럼...
    _tileSprites.emplace('H', makeTileSprite(' ', Color::Black));   // 위에 아이템이 그려지고
    _tileSprites.emplace('T', makeTileSprite(' ', Color::Black));   // 습득하면 바닥으로 보여야 함
}
