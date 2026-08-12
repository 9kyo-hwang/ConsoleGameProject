#include "pch.h"
#include "OverworldMap.h"
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

bool OverworldMap::Load(const FilePath& tileMapPath, const FilePath& blockingMapPath, std::string& error)
{
    if (_loaded)
    {
        return true;
    }

    Grid cells{};
    if (!ParseTileMap(tileMapPath, cells, error))
    {
        _loaded = false;
        return false;
    }

    if (!ParseBlockingMap(blockingMapPath, cells, error))
    {
        _loaded = false;
        return false;
    }

    _cells = std::move(cells);
    _loaded = true;
    return true;
}

TileId OverworldMap::GetTileId(int x, int y) const
{
    if (OutOfBound(x, y)) return InvalidTileId;
    return _cells[y][x].id;
}

/// <summary>
/// 플레이어의 BoxCollider가 조금이라도 걸치는 타일 중 하나라도 막혀있으면 이동 불가
/// </summary>
/// <param name="boxColliderPosition">전체 맵 기준 BoxComponent 좌상단 좌표</param>
/// <param name="boxColliderSize">BoxComponent 크기</param>
/// <param name="tileSize">RenderScale이 적용된 타일 하나의 크기(픽셀)</param>
/// <returns></returns>
bool OverworldMap::CanOccupyWorldRect(const Vector2& boxColliderPosition, const Vector2& boxColliderSize, const Vector2& tileSize) const
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

bool OverworldMap::ParseTileMap(const FilePath& path, Grid& output, std::string& errorMessage)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        errorMessage = "TileMap file could not be opened.";
        return false;
    }

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

            output[row][col].id = *id;
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

bool OverworldMap::ParseBlockingMap(const FilePath& path, Grid& output, std::string& errorMessage)
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
                output[row][col].walkable = true;
            }
            else if (line[col] == 'X')
            {
                output[row][col].walkable = false;
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
    return x < 0 || x >= Width || y < 0 || y >= Height;
}

bool OverworldMap::IsWalkable(int x, int y) const
{
    if (OutOfBound(x, y)) return false;
    return _cells[y][x].walkable;
}
