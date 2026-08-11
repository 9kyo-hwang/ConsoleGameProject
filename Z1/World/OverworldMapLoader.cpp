#include "pch.h"
#include "OverworldMapLoader.h"
#include <fstream>
#include <sstream>

bool OverworldMapData::OutOfBound(int x, int y) const
{
    return x < 0 || x >= OverworldMapData::Width || y < 0 || y >= OverworldMapData::Height;
}

TileId OverworldMapData::GetTileId(int x, int y) const
{
    if (OutOfBound(x, y)) return InvalidTileId;
    return tileIds[Index(x, y)];
}

bool OverworldMapData::IsWalkable(int x, int y) const
{
    if (OutOfBound(x, y)) return false;
    return walkable[Index(x, y)];
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

                output.tileIds[row * OverworldMapData::Width + col] = *id;
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
                    output.walkable[row * OverworldMapData::Width + col] = true;
                }
                else if (line[col] == 'X')
                {
                    output.walkable[row * OverworldMapData::Width + col] = false;
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

std::optional<RoomDefinition> OverworldMapLoader::ExtractRoom(int roomX, int roomY, std::string& errorMessage) const
{ 
    if (!_loaded)
    {
        errorMessage = "Overworld map has not been loaded.";
        return std::nullopt;
    }

    if (roomX < 0 || roomX >= OverworldMapData::RoomColumns ||
        roomY < 0 || roomY >= OverworldMapData::RoomRows)
    {
        errorMessage = "Room coordinate is outside the overworld.";
        return std::nullopt;
    }

    RoomDefinition room;
    room.roomX = roomX;
    room.roomY = roomY;

    // 전체 Map에서 해당 Room의 좌표로 convert
    const int sourceX = roomX * RoomDefinition::Width;
    const int sourceY = roomY * RoomDefinition::Height;

    for (int localY = 0; localY < RoomDefinition::Height; ++localY)
    {
        for (int localX = 0; localX < RoomDefinition::Width; ++localX)
        {
            const int worldX = sourceX + localX;
            const int worldY = sourceY + localY;

            const int index = RoomDefinition::Index(localX, localY);
            room.tileIds[index] = _data.GetTileId(worldX, worldY);
            room.walkable[index] = _data.IsWalkable(worldX, worldY);
        }
    }

    return room;
}
