#pragma once

#include <cstdint>
#include <memory>
#include <Render/Sprite.h>
#include <unordered_map>

using TileId = std::uint8_t;    // 00, 01, ..., 9c, 9d. 지형 구분 Id
constexpr TileId InvalidTileId = 0xff;

/*
* 예시
* Tile ground
* {
*   0x02,
*   groundSprite,
*   true,
*   false,
*   false,
*   false
* }
*/

struct TileDefinition
{
    TileId id = InvalidTileId;
    std::shared_ptr<const Craft::Sprite> sprite;

    bool walkable = false;
    bool isExit = false;
};

// TileId로 TileDefinition을 조회하는 저장소
class TileCatalog
{
public:
    bool Find(TileId id, TileDefinition& outTile) const;
    void Register(TileDefinition tile);

private:
    std::unordered_map<TileId, TileDefinition> _tiles;
};