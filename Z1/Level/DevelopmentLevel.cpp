#include "pch.h"
#include "DevelopmentLevel.h"
#include <Actor/CollisionTestActor.h>
#include <Render/Renderer.h>
#include <Core/Input.h>
#include <Component/BoxComponent.h>
#include <Component/SpriteRendererComponent.h>

using namespace Craft;

namespace
{
    // TEMP: 아직 Sprite가 없어 TileId 2글자 16진수 그대로 표시
    std::string MakeTileIdRow(const RoomDefinition& room, int y)
    {
        const char* const Digits = "0123456789ABCDEF";
        std::string result;
        result.reserve(RoomDefinition::Width * 2);

        for (int x = 0; x < RoomDefinition::Width; ++x)
        {
            const TileId id = room.GetTileId(x, y);
            result.push_back(Digits[(id >> 4) & 0x0f]);
            result.push_back(Digits[id & 0x0f]);
        }

        return result;
    }

    // 임시 타일 이미지 생성 헬퍼
    /*
    * 임시 Tile 정보
    * 바닥: ..
    * 벽: ##
    * 물: ~~
    * 나무: TT
    * 계단: <>
    * 미등록: ??
    */
    std::shared_ptr<const Sprite> MakeTileSprite(char left, char right, WORD attribute)
    {
        // 2 x 1
        std::vector<SpriteCell> cells
        {
            SpriteCell(left, attribute, false),
            SpriteCell(right, attribute, false)
        };

        return std::make_shared<const Sprite>(Vector2(2, 1), std::move(cells));
    }

    TileCatalog CreateCatalog()
    {
        TileCatalog catalog;
        catalog.Register(TileDefinition
            {
                0x02,
                MakeTileSprite(' ', ' ', (WORD)Color::White),
                true,
                false
            }
        );

        // 원래는 빈 검은 공간이지만 구분을 위해
        catalog.Register(TileDefinition
            {
                0x12,
                MakeTileSprite('[', ']', (WORD)Color::White),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x14,
                MakeTileSprite('=', '=', (WORD)Color::Blue),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x28,
                MakeTileSprite('#', '#', (WORD)Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x29,
                MakeTileSprite('#', '#', (WORD)Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x2A,
                MakeTileSprite('#', '#', (WORD)Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x28,
                MakeTileSprite('#', '#', (WORD)Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x29,
                MakeTileSprite('#', '#', (WORD)Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x3C,
                MakeTileSprite('#', '#', (WORD)Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x3D,
                MakeTileSprite('#', '#', (WORD)Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x3E,
                MakeTileSprite('#', '#', (WORD)Color::Yellow),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x30,
                MakeTileSprite('T', 'T', (WORD)Color::Green),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x42,
                MakeTileSprite('T', 'T', (WORD)Color::Green),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x43,
                MakeTileSprite('T', 'T', (WORD)Color::Green),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x44,
                MakeTileSprite('T', 'T', (WORD)Color::Green),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x50,
                MakeTileSprite('~', '~', (WORD)Color::Blue),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x51,
                MakeTileSprite('~', '~', (WORD)Color::Blue),
                false,
                false
            }
        );

        catalog.Register(TileDefinition
            {
                0x52,
                MakeTileSprite('~', '~', (WORD)Color::Blue),
                false,
                false
            }
        );

        return catalog;
    }
}

DevelopmentLevel::DevelopmentLevel()
{
}

void DevelopmentLevel::OnInitialized()
{
    Level::OnInitialized();
}

void DevelopmentLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_attempted)
    {
        InitializeMapTest();
    }

    if (_testA)
    {
        return;
    }

    _testA = SpawnActor<CollisionTestActor>(
        Vector2(14, 5),
        Vector2(2, 1),
        Vector2::Zero
    );

    _testA->AddComponent<SpriteRendererComponent>("PP");
}

void DevelopmentLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (!_testA) return;

    Vector2 delta = Vector2::Zero;
    if (Input::Get().GetKeyDown('A')) delta.x = -1;
    if (Input::Get().GetKeyDown('D')) delta.x = 1;
    if (Input::Get().GetKeyDown('W')) delta.y = -1;
    if (Input::Get().GetKeyDown('S')) delta.y = 1;

    if (delta == Vector2::Zero) return;

    const Vector2 candidate = _testA->GetPosition() + delta;
    if (CanMove(candidate))
    {
        _testA->SetPosition(candidate);
    }
}

void DevelopmentLevel::Draw()
{
    // 왜 먼저 제출?
    if (_roomSprite)
    {
        Renderer::Get().Submit("Room Sprite: READY", Vector2(35, 4), Color::White, 100);
        Renderer::Get().Submit(_roomSprite, Vector2(0, 3), 0);
    }

    Level::Draw();

    Renderer::Get().Submit("[Development Level]", Vector2::Zero);
    Renderer::Get().Submit("Map: " + _mapStatus, Vector2(35, 1));
    //Renderer::Get().Submit("CollisionCount A: " + std::to_string(_testA->GetCollisionCount()), Vector2(35, 2));
    //Renderer::Get().Submit("CollisionCount B: " + std::to_string(_testB->GetCollisionCount()), Vector2(35, 3));
}

void DevelopmentLevel::InitializeMapTest()
{
    _attempted = true;

    const FilePath basePath = "../Content/Z1/Maps/Overworld";
    std::string error;
    const bool loaded = _loader.Load(basePath / "TileMap.txt", basePath / "BlockingMap.txt", error);

    if (!loaded)
    {
        _mapStatus = "LOAD FAILED";
        return;
    }

    auto room = _loader.ExtractRoom(7, 7, error);
    if (!room)
    {
        _mapStatus = "ROOM EXTRACT FAILED";
        return;
    }

    _room = std::move(room);

    const bool tile = _room->GetTileId(0, 0) == 0x43;
    const bool blocked = !_room->IsWalkable(0, 0);
    const bool walkable = _room->IsWalkable(7, 0);

    if (tile && blocked && walkable)
    {
        _mapStatus = "PASS";
    }
    else
    {
        _mapStatus = "DATA MISMATCH";
    }

    _tileCatalog = CreateCatalog();
    if (_room.has_value())
    {
        BuildRoomSprite();
    }
}

void DevelopmentLevel::BuildRoomSprite()
{
    if (!_room.has_value())
    {
        return;
    }

    //constexpr int CellsPerTileX = 2;
    constexpr int RoomPixelWidth = RoomDefinition::Width << 1;   // (2x1)에 대응
    constexpr int RoomPixelHeight = RoomDefinition::Height;   // (2x1)에 대응

    std::vector<SpriteCell> cells(RoomPixelWidth * RoomPixelHeight, SpriteCell());
    bool hasMissingTile = false;

    // Room -> Tile -> Sprite
    for (int tileY = 0; tileY < RoomDefinition::Height; ++tileY)
    {
        for (int tileX = 0; tileX < RoomDefinition::Width; ++tileX)
        {
            const TileId id = _room->GetTileId(tileX, tileY);
            TileDefinition tile;
            if (!_tileCatalog.Find(id, tile) || !tile.sprite)
            {
                hasMissingTile = true;

                const int cellX = tileX << 1;
                const int cellY = tileY;
                const int cellIndex = cellY * RoomPixelWidth + cellX;

                cells[cellIndex] = SpriteCell{ '?', (WORD)Color::White, false };
                cells[cellIndex + 1] = SpriteCell{ '?', (WORD)Color::White, false };

                continue;
            }

            const Vector2 tileSize = tile.sprite->GetSize();
            if (tileSize.x != 2 || tileSize.y != 1)
            {
                hasMissingTile = true;
                continue;
            }

            const int cellX = tileX << 1;
            const int cellY = tileY;
            const int cellIndex = cellY * RoomPixelWidth + cellX;

            cells[cellIndex] = tile.sprite->GetCell(0, 0);
            cells[cellIndex + 1] = tile.sprite->GetCell(1, 0);
        }
    }

    _roomSprite = std::make_shared<const Sprite>(Vector2(RoomPixelWidth, RoomPixelHeight), std::move(cells));
}

bool DevelopmentLevel::CanMove(const Craft::Vector2& candidate) const
{
    if (!_room || !_testA)
    {
        return false;
    }

    auto box = _testA->GetComponent<BoxComponent>();
    const Vector2 size = box->GetSize();
    const Vector2 offset = box->GetOffset();

    const Vector2 origin(0, 3);

    const int localLeft = candidate.x + offset.x - origin.x;
    const int localTop = candidate.y + offset.y - origin.y;
    const int localRight = localLeft + size.x - 1;
    const int localBottom = localTop + size.y - 1;

    if (localLeft < 0 || localTop < 0 || localRight >= RoomDefinition::Width * 2 || localBottom >= RoomDefinition::Height)
    {
        return false;
    }

    return _room->CanOccupyTiles(localLeft / 2, localTop, localRight / 2, localBottom);
}
