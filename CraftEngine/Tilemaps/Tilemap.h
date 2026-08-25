#pragma once
#include <Core/Core.h>
#include <Render/Sprite.h>
#include <memory>
#include <Math/Vector2.h>
#include <Math/Box2D.h>
#include <vector>

namespace Craft
{
    struct CRAFT_API Tile
    {
        std::shared_ptr<const Sprite> sprite = nullptr;
        bool blocked = false;
    };

    class CRAFT_API Tilemap
    {
    public:
        Tilemap(Vector2 size, Vector2 cellSize);

        inline Vector2 GetSize() const { return _size; }           // 셀 단위 타일맵 크기
        inline Vector2 GetCellSize() const { return _cellSize; }   // 각 셀 크기

        bool IsInBounds(Vector2 cell) const;  // 해당 셀이 Tilemap 범위에 들어오는지
        
        inline Vector2 CellToWorld(Vector2 cell) const { return cell * _cellSize; }    // 논리 -> 실제 픽셀
        inline Vector2 WorldToCell(Vector2 world) const { return Vector2(world.x / _cellSize.x, world.y / _cellSize.y); }  // 픽셀 -> 논리
        inline Box2D GetCellBounds(Vector2 cell) const { return Box2D(CellToWorld(cell), _cellSize); } // 픽셀 기준 좌표, 픽셀 기준 크기

        const Tile* GetTile(Vector2 cell) const;
        bool SetTile(Vector2 cell, Tile tile);

        bool CanPlaceBox(const Box2D& box) const;   // TilemapCollider2D 대신...
        std::shared_ptr<const Sprite> BuildSprite(Vector2 origin, Vector2 count) const; // origin 위치에서 count개의 타일로 구성된 Sprite

    private:
        size_t GetIndex(Vector2 cell) const;

    private:
        Vector2 _size;      // 타일맵 크기(타일 개수 단위)
        Vector2 _cellSize;  // 논리 타일 하나의 월드/콘솔 크기. Z1에선 10 x 5
        std::vector<Tile> _tiles;
    };
}


