#pragma once

#include <cstdint>
#include <memory>
#include <Render/Sprite.h>
#include <unordered_map>

using TileId = std::uint8_t;    // 00, 01, ..., 9c, 9d. 지형 구분 Id
constexpr TileId InvalidTileId = 0xff;

// TileId - Sprite 저장소
class TileSpriteCatalog
{
public:
    bool TryGet(TileId id, std::shared_ptr<const Craft::Sprite>& sprite) const;
    void Add(TileId id, std::shared_ptr<const Craft::Sprite> sprite);

private:
    std::unordered_map<TileId, std::shared_ptr<const Craft::Sprite>> _tiles;
};