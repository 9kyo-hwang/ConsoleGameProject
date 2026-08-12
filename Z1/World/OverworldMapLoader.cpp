#include "pch.h"
#include "OverworldMapLoader.h"
#include <fstream>
#include <sstream>

using namespace Craft;

bool OverworldMapData::OutOfBound(int x, int y) const
{
    return x < 0 || x >= OverworldMapData::Width || y < 0 || y >= OverworldMapData::Height;
}

TileId OverworldMapData::GetTileId(int x, int y) const
{
    if (OutOfBound(x, y)) return InvalidTileId;
    return _tileIds[Index(x, y)];
}

void OverworldMapData::SetTileId(int x, int y, TileId id)
{
    assert(!OutOfBound(x, y));
    assert(id != InvalidTileId);

    _tileIds[y * Width + x] = id;
}

bool OverworldMapData::IsWalkable(int x, int y) const
{
    if (OutOfBound(x, y)) return false;
    return _walkables[Index(x, y)];
}

void OverworldMapData::SetWalkable(int x, int y, bool walkable)
{
    assert(!OutOfBound(x, y));

    _walkables[y * Width + x] = walkable;
}

/// <summary>
/// 플레이어의 BoxCollider가 조금이라도 걸치는 타일 중 하나라도 막혀있으면 이동 불가
/// </summary>
/// <param name="boxColliderPosition">전체 맵 기준 BoxComponent 좌상단 좌표</param>
/// <param name="boxColliderSize">BoxComponent 크기</param>
/// <param name="tileSize">RenderScale이 적용된 타일 하나의 크기(픽셀)</param>
/// <returns></returns>
bool OverworldMapData::CanOccupyWorldRect(const Vector2& boxColliderPosition, const Vector2& boxColliderSize, const Vector2& tileSize) const
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

    bool ParseTileMap(const FilePath& path, OverworldMapData& output, std::string& errorMessage)
    {
        std::ifstream file(path);
        if (!file.is_open())
        {
            errorMessage = "TileMap file could not be opened.";
            return false;
        }

        std::string line;
        for (int row = 0; row < OverworldMapData::Height; ++row)
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

            for (int col = 0; col < OverworldMapData::Width; ++col)
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

                output.SetTileId(col, row, *id);
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

    bool ParseBlockingMap(const FilePath& path, OverworldMapData& output, std::string& errorMessage)
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
        for (int row = 0; row < OverworldMapData::Height; ++row)
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

            if (line.size() != OverworldMapData::Width)
            {
                errorMessage = "BlockingMap row must contains 256 characters.";
                return false;
            }

            for (int col = 0; col < OverworldMapData::Width; ++col)
            {
                if (line[col] == '.')
                {
                    output.SetWalkable(col, row, true);
                }
                else if (line[col] == 'X')
                {
                    output.SetWalkable(col, row, false);
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
}

bool OverworldMapLoader::Load(const FilePath& tileMapPath, const FilePath& blockingMapPath, std::string& errorMessage)
{
    OverworldMapData data;
    if (!ParseTileMap(tileMapPath, data, errorMessage))
    {
        return false;
    }

    if (!ParseBlockingMap(blockingMapPath, data, errorMessage))
    {
        return false;
    }

    _data = std::move(data);
    _loaded = true;
    return true;
}

bool OverworldMapLoader::CanOccupyWorldRect(const Vector2& boxColliderPosition, const Vector2& boxColliderSize, const Vector2& tileSize) const
{
    if (!_loaded) return false;
    return _data.CanOccupyWorldRect(boxColliderPosition, boxColliderSize, tileSize);
}

std::optional<RoomData> OverworldMapLoader::ExtractRoom(RoomCoordinate room, std::string& errorMessage) const
{ 
    if (!_loaded)
    {
        errorMessage = "Overworld map has not been loaded.";
        return std::nullopt;
    }

    if (room.x < 0 || room.x >= OverworldMapData::RoomColumns ||
        room.y < 0 || room.y >= OverworldMapData::RoomRows)
    {
        errorMessage = "Room coordinate is outside the overworld.";
        return std::nullopt;
    }

    RoomData data;

    // 전체 Map에서 해당 Room의 좌표로 convert
    const int sourceX = room.x * RoomData::Width;
    const int sourceY = room.y * RoomData::Height;

    for (int localY = 0; localY < RoomData::Height; ++localY)
    {
        for (int localX = 0; localX < RoomData::Width; ++localX)
        {
            const int mapX = sourceX + localX;
            const int mapY = sourceY + localY;

            const int index = RoomData::Index(localX, localY);
            data.tileIds[index] = _data.GetTileId(mapX, mapY);
        }
    }

    return data;
}
