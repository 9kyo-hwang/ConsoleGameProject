#include "pch.h"
#include "OverworldLevel.h"
#include <Actor/Player.h>
#include <Core/Input.h>
#include <Render/Renderer.h>
#include <Component/BoxComponent.h>

using namespace Craft;

namespace
{
    const Vector2 WorldRenderScale(5, 3);
    const Vector2 RoomOrigin(0, 3);

    std::shared_ptr<const Sprite> MakeTileSprite(char glyph, Color color)
    {
        std::vector<SpriteCell> cells
        {
            SpriteCell(glyph, (WORD)color, false),
        };

        return std::make_shared<const Sprite>(Vector2::One, std::move(cells));
    }

    TileCatalog CreateCatalog()
    {
        TileCatalog catalog;
        catalog.Register(TileDefinition
            {
                0x02,
                MakeTileSprite('.', Color::White),
                true,
                false
            }
        );

        // 원래는 빈 검은 공간이지만 구분을 위해
        catalog.Register(TileDefinition
            {
                0x12,
                MakeTileSprite(' ', Color::White),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x14,
                MakeTileSprite('=', Color::Blue),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x28,
                MakeTileSprite('#', Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x29,
                MakeTileSprite('#', Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x2A,
                MakeTileSprite('#', Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x3C,
                MakeTileSprite('#', Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x3D,
                MakeTileSprite('#', Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x3E,
                MakeTileSprite('#', Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x30,
                MakeTileSprite('T', Color::Green),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x42,
                MakeTileSprite('T', Color::Green),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x43,
                MakeTileSprite('T', Color::Green),
                false,
                false
            }
        );

catalog.Register(TileDefinition
    {
        0x44,
        MakeTileSprite('T', Color::Green),
        false,
        false
    }
);

catalog.Register(TileDefinition
    {
        0x50,
        MakeTileSprite('~', Color::Blue),
        false,
        false
    }
);

catalog.Register(TileDefinition
    {
        0x51,
        MakeTileSprite('~', Color::Blue),
        false,
        false
    }
);

catalog.Register(TileDefinition
    {
        0x52,
        MakeTileSprite('~', Color::Blue),
        false,
        false
    }
);

return catalog;
    }
}

OverworldLevel::OverworldLevel()
{
}

void OverworldLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_loaded)
    {
        LoadMap();
    }

    if (!_player)
    {
        _player = SpawnActor<Player>(RoomOrigin + Vector2(7, 2), WorldRenderScale);
    }
}

void OverworldLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (!_player) return;

    Vector2 delta = Vector2::Zero;
    if (Input::Get().GetKeyDown('A')) delta.x = -1;
    if (Input::Get().GetKeyDown('D')) delta.x = 1;
    if (Input::Get().GetKeyDown('W')) delta.y = -1;
    if (Input::Get().GetKeyDown('S')) delta.y = 1;

    if (delta == Vector2::Zero) return;

    const Vector2 candidate = _player->GetPosition() + delta;
    if (CanMove(candidate))
    {
        _player->MoveBy(delta);
    }
}

void OverworldLevel::Draw()
{
    if (_roomSprite)
    {
        Renderer::Get().Submit(_roomSprite, RoomOrigin, WorldRenderScale, 0);
    }

    Level::Draw();

    Renderer::Get().Submit("[Overworld Level]", Vector2::Zero);
}

void OverworldLevel::LoadMap()
{
    _loaded = true;

    const FilePath basePath = "../Content/Z1/Maps/Overworld";
    std::string error;
    if(!_loader.Load(basePath / "TileMap.txt", basePath / "BlockingMap.txt", error))
    {
        return;
    }

    auto room = _loader.ExtractRoom(7, 7, error);
    if (!room)
    {
        return;
    }

    _room = std::move(room);

    _tileCatalog = CreateCatalog();
    if (_room.has_value())
    {
        BuildRoomSprite();
    }
}

void OverworldLevel::BuildRoomSprite()
{
    if (!_room.has_value())
    {
        return;
    }

    //constexpr int CellsPerTileX = 2;
    constexpr int RoomPixelWidth = RoomDefinition::Width;
    constexpr int RoomPixelHeight = RoomDefinition::Height;

    std::vector<SpriteCell> cells(RoomPixelWidth * RoomPixelHeight, SpriteCell());

    // Room -> Tile -> Sprite
    for (int tileY = 0; tileY < RoomDefinition::Height; ++tileY)
    {
        for (int tileX = 0; tileX < RoomDefinition::Width; ++tileX)
        {
            const TileId id = _room->GetTileId(tileX, tileY);
            TileDefinition tile;
            if (!_tileCatalog.Find(id, tile) || !tile.sprite)
            {
                const int cellX = tileX;
                const int cellY = tileY;
                const int cellIndex = cellY * RoomPixelWidth + cellX;

                cells[cellIndex] = SpriteCell{ '?', (WORD)Color::White, false };
                continue;
            }

            const Vector2 tileSize = tile.sprite->GetSize();
            if (tileSize != Vector2::One)
            {
                continue;
            }

            const int cellX = tileX;
            const int cellY = tileY;
            const int cellIndex = cellY * RoomPixelWidth + cellX;

            cells[cellIndex] = tile.sprite->GetCell(0, 0);
        }
    }

    _roomSprite = std::make_shared<const Sprite>(Vector2(RoomPixelWidth, RoomPixelHeight), std::move(cells));
}

bool OverworldLevel::CanMove(const Craft::Vector2& candidate) const
{
    if (!_room || !_player)
    {
        return false;
    }

    auto box = _player->GetComponent<BoxComponent>();
    const Vector2 size = box->GetSize();
    const Vector2 offset = box->GetOffset();

    const int left = candidate.x + offset.x - RoomOrigin.x;
    const int top = candidate.y + offset.y - RoomOrigin.y;
    const int right = left + size.x - 1;
    const int bottom = top + size.y - 1;

    if (left < 0 || top < 0 || right >= RoomDefinition::Width || bottom >= RoomDefinition::Height)
    {
        return false;
    }

    return _room->CanOccupyTiles(left, top, right, bottom);
}
