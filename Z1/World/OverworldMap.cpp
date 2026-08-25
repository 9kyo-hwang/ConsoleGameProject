#include "pch.h"
#include "OverworldMap.h"
#include <World/MapGeometry.h>
#include <fstream>

using namespace Craft;

namespace
{
    std::optional<TileId> ParseHexTileId(const std::string& token)
    {
        /*
        * 토큰이 비어있지 않은가
        * 16진수 변환이 가능한가
        * 값이 0x00 ~ 0xff 사이인가
        * 원본 형식대로 2자리인가
        */

        if (token.size() != 2)
        {
            return std::nullopt;
        }

        unsigned int value = 0;

        // 문자열 2자리 16진수를 추출
        const auto result = std::from_chars(token.data(), token.data() + token.size(), value, 16);
        if (result.ec != std::errc{} ||
            result.ptr != token.data() + token.size() ||
            value > 0xff)
        {
            return std::nullopt;
        }

        return static_cast<TileId>(value);
    };
}

OverworldMap::OverworldMap()
    : _tilemap(Vector2(Width, Height), TileCellSize)
{
    InitializeTileSprites();
}

bool OverworldMap::Load(const FilePath& tileMapPath, const FilePath& blockingMapPath, std::string& error)
{
    if (_loaded)
    {
        return true;
    }

    std::vector<TileId> tileIds;
    if (!ParseTileMap(tileMapPath, tileIds, error))
    {
        _loaded = false;
        return false;
    }

    std::vector<bool> blocked;
    if (!ParseBlockingMap(blockingMapPath, blocked, error))
    {
        _loaded = false;
        return false;
    }

    Tilemap tilemap(Vector2(Width, Height), TileCellSize);

    for (int y = 0; y < Height; ++y)
    {
        for (int x = 0; x < Width; ++x)
        {
            const size_t index = y * Width + x;
            const TileId id = tileIds[index];

            const auto found = _tileSprites.find(id);
            if (found == _tileSprites.end())
            {
                // 음, fallback으로 '?' 처리 해줘야 하나?
                error = "_tileSprites에 없는 id";
                return false;
            }

            const bool set = tilemap.SetTile(Vector2(x, y), Tile(found->second, blocked[index]));
            assert(set);
        }
    }

    _loaded = true;
    _tileIds = std::move(tileIds);
    _tilemap = std::move(tilemap);

    return true;
}

TileId OverworldMap::GetTileId(int x, int y) const
{
    if (OutOfBound(x, y)) return InvalidTileId;
    return _tileIds[y * _tilemap.GetSize().x + x];
}

std::shared_ptr<const Craft::Sprite> OverworldMap::BuildRoomSprite(Vector2 roomOrigin, Vector2 roomSize) const
{
    return _tilemap.BuildSprite(roomOrigin, roomSize);
}

/// <summary>
/// 플레이어의 BoxCollider가 조금이라도 걸치는 타일 중 하나라도 막혀있으면 이동 불가
/// </summary>
bool OverworldMap::CanPlaceBox(const Box2D& box) const
{
    return _tilemap.CanPlaceBox(box);
}

void OverworldMap::InitializeTileSprites()
{
    // TileCellSize만큼 반복해야 해서
    const auto makeTileSprite = [](char glyph, Color color)
        {
            return Sprite::Create(TileCellSize, glyph, color);
        };

    // 0x00 ~ 0x11: 갈색/회색 절벽·나무·평지
    _tileSprites.emplace(0x00, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x01, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x02, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x03, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x04, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x05, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x06, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x07, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x08, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x09, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x0A, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x0B, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x0C, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x0D, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x0E, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x0F, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x10, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x11, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x12, makeTileSprite(' ', Color::DarkYellow)); // 예외: 빈칸

    // 0x14 ~ 0x25: 다리, 나무, 절벽, 특수 구조물
    _tileSprites.emplace(0x14, makeTileSprite('=', Color::DarkYellow));
    _tileSprites.emplace(0x15, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x16, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x17, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x18, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x19, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x1A, makeTileSprite('=', Color::DarkYellow));
    _tileSprites.emplace(0x1B, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x1C, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x1D, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x1E, makeTileSprite('O', Color::DarkYellow));
    _tileSprites.emplace(0x1F, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x20, makeTileSprite('=', Color::DarkYellow));
    _tileSprites.emplace(0x21, makeTileSprite('+', Color::DarkYellow));
    _tileSprites.emplace(0x22, makeTileSprite('O', Color::DarkYellow));
    _tileSprites.emplace(0x23, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x24, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x25, makeTileSprite('#', Color::DarkYellow));

    // 0x28 ~ 0x39: 산맥, 숲, 동굴/구조물
    _tileSprites.emplace(0x28, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x29, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x2A, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x2B, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x2C, makeTileSprite('O', Color::DarkYellow));
    _tileSprites.emplace(0x2D, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x2E, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x2F, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x30, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x31, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x32, makeTileSprite('O', Color::DarkYellow));
    _tileSprites.emplace(0x33, makeTileSprite('T', Color::DarkYellow));
    _tileSprites.emplace(0x34, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x35, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x36, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x37, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x38, makeTileSprite('O', Color::DarkYellow));
    _tileSprites.emplace(0x39, makeTileSprite('#', Color::DarkYellow));

    // 0x3C ~ 0x4D: 산맥, 숲, 물가
    _tileSprites.emplace(0x3C, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x3D, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x3E, makeTileSprite('#', Color::DarkYellow));
    _tileSprites.emplace(0x3F, makeTileSprite('=', Color::DarkRed)   );
    _tileSprites.emplace(0x40, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x41, makeTileSprite('=', Color::DarkRed)   );
    _tileSprites.emplace(0x42, makeTileSprite('T', Color::Green)     );
    _tileSprites.emplace(0x43, makeTileSprite('T', Color::Green)     );
    _tileSprites.emplace(0x44, makeTileSprite('T', Color::Green)     );
    _tileSprites.emplace(0x45, makeTileSprite('T', Color::Green)     );
    _tileSprites.emplace(0x46, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x47, makeTileSprite('T', Color::Green)     );
    _tileSprites.emplace(0x48, makeTileSprite('#', Color::Gray)      );
    _tileSprites.emplace(0x49, makeTileSprite('#', Color::Gray)      );
    _tileSprites.emplace(0x4A, makeTileSprite('#', Color::Gray)      );
    _tileSprites.emplace(0x4B, makeTileSprite('#', Color::Gray)      );
    _tileSprites.emplace(0x4C, makeTileSprite('=', Color::Blue)      );
    _tileSprites.emplace(0x4D, makeTileSprite('.', Color::Gray));

    // 0x50 ~ 0x61: 팔레트별 평지·물·바위 지형
    _tileSprites.emplace(0x50, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x51, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x52, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x53, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x54, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x55, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x56, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x57, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x58, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x59, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x5A, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x5B, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x5C, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x5D, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x5E, makeTileSprite('~', Color::Blue)      );
    _tileSprites.emplace(0x5F, makeTileSprite('.', Color::Gray)      );
    _tileSprites.emplace(0x60, makeTileSprite('.', Color::Gray));
    _tileSprites.emplace(0x61, makeTileSprite('.', Color::Gray));

    // 0x64 ~ 0x75: 위 지형의 하단/연결 조각
    _tileSprites.emplace(0x64, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x65, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x66, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x67, makeTileSprite('.', Color::DarkYellow) );
    _tileSprites.emplace(0x68, makeTileSprite('.', Color::DarkYellow) );
    _tileSprites.emplace(0x69, makeTileSprite('.', Color::DarkYellow) );
    _tileSprites.emplace(0x6A, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x6B, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x6C, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x6D, makeTileSprite('.', Color::DarkYellow) );
    _tileSprites.emplace(0x6E, makeTileSprite('.', Color::DarkYellow) );
    _tileSprites.emplace(0x6F, makeTileSprite('.', Color::DarkYellow) );
    _tileSprites.emplace(0x70, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x71, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x72, makeTileSprite('~', Color::Blue) );
    _tileSprites.emplace(0x73, makeTileSprite('.', Color::Gray) );
    _tileSprites.emplace(0x74, makeTileSprite('.', Color::Gray) );
    _tileSprites.emplace(0x75, makeTileSprite('.', Color::Gray));

    // 0x78 ~ 0x89: 위 지형의 하단/연결 조각
    _tileSprites.emplace(0x78, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x79, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x7A, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x7B, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x7C, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x7D, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x7E, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x7F, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x80, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x81, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x82, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x83, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x84, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x85, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x86, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x87, makeTileSprite('.', Color::Gray));
    _tileSprites.emplace(0x88, makeTileSprite('.', Color::Gray));
    _tileSprites.emplace(0x89, makeTileSprite('.', Color::Gray));

    // 0x8C ~ 0x9D: 물가 모서리, 다리, 동굴/구조물
    _tileSprites.emplace(0x8C, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x8D, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x8E, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x8F, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x90, makeTileSprite('~', Color::Blue));
    _tileSprites.emplace(0x91, makeTileSprite('=', Color::DarkRed));
    _tileSprites.emplace(0x92, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x93, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x94, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x95, makeTileSprite('.', Color::DarkYellow));
    _tileSprites.emplace(0x96, makeTileSprite('O', Color::White));
    _tileSprites.emplace(0x97, makeTileSprite('=', Color::Green));
    _tileSprites.emplace(0x98, makeTileSprite('.', Color::Gray));
    _tileSprites.emplace(0x99, makeTileSprite('.', Color::Gray));
    _tileSprites.emplace(0x9A, makeTileSprite('.', Color::Gray));
    _tileSprites.emplace(0x9B, makeTileSprite('.', Color::Gray));
    _tileSprites.emplace(0x9C, makeTileSprite('O', Color::DarkRed));
    _tileSprites.emplace(0x9D, makeTileSprite('O', Color::White));
}

bool OverworldMap::ParseTileMap(const FilePath& path, std::vector<TileId>& tileIds, std::string& errorMessage)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        errorMessage = "TileMap file could not be opened.";
        return false;
    }

    tileIds.resize(Width * Height);

    std::string line;
    for (int row = 0; row < Height; ++row)
    {
        if (!std::getline(file, line))
        {
            errorMessage = "TileMap has fewer than 88 rows.";
            return false;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::istringstream stream(line);

        for (int col = 0; col < Width; ++col)
        {
            std::string token;
            if (!(stream >> token))
            {
                errorMessage = "TileMap row has fewer than 256 tokens";
                return false;
            }

            std::optional<TileId> id = ParseHexTileId(token);
            if (!id)
            {
                errorMessage = "TileMap contains an invalid hexadecimal TileId.";
                return false;
            }

            tileIds[row * Width + col] = *id;
        }

        std::string extraToken;
        if (stream >> extraToken)
        {
            errorMessage = "TileMap has more than 256 tokens.";
            return false;
        }
    }

    if (std::getline(file, line))
    {
        errorMessage = "TileMap has more than 88 rows.";
        return false;
    }

    return true;
}

bool OverworldMap::ParseBlockingMap(const FilePath& path, std::vector<bool>& blocked, std::string& errorMessage)
{
    /*
    * 검증
    - 88행
    - 행 길이 256
    - 각 문자는 . 또는 X
    */

    std::ifstream file(path);
    if (!file.is_open())
    {
        errorMessage = "BlockingMap file could not be opened.";
        return false;
    }

    blocked.resize(Width * Height);

    std::string line;
    for (int row = 0; row < Height; ++row)
    {
        if (!std::getline(file, line))
        {
            errorMessage = "BlockingMap has fewer than 88 rows.";
            return false;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.size() != Width)
        {
            errorMessage = "BlockingMap row must contains 256 characters.";
            return false;
        }

        for (int col = 0; col <Width; ++col)
        {
            if (line[col] == '.')
            {
                blocked[row * Width + col] = false;
            }
            else if (line[col] == 'X')
            {
                blocked[row * Width + col] = true;
            }
            else
            {
                errorMessage = "BlockingMap map contains an invalid character.";
                return false;
            }
        }
    }

    if (std::getline(file, line))
    {
        errorMessage = "BlockingMap has more than 88 rows.";
        return false;
    }

    return true;
}

bool OverworldMap::OutOfBound(int x, int y) const
{
    return !_tilemap.IsInBounds(Vector2(x, y));
}

bool OverworldMap::IsWalkable(int x, int y) const
{
    if (const Tile* tile = _tilemap.GetTile(Vector2(x, y)))
    {
        return !tile->blocked;
    }

    return false;
}
