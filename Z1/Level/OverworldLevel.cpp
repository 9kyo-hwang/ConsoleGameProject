#include "pch.h"
#include "OverworldLevel.h"
#include <Actor/Player.h>
#include <Core/Input.h>
#include <Render/Renderer.h>
#include <Component/BoxComponent.h>

using namespace Craft;

/*
* Room의 타일 좌표: 16 x 11
* Actor 위치 및 Box 크기: 월드 Cell 단위
* Room 하나의 실제 월드 크기: (16 x 11) x WorldRenderScale = (80 x 33)
* RoomOrigin: 월드 Cell 단위
* 렌더러에 전달하는 Actor 위치: 콘솔 Cell과 1:1인 월드 좌표
* 
* 예: 타일 (7, 4)
* - LTRB: 35, 12, 39, 14
*/

namespace
{
    const Vector2 MapTileSize(5, 3);
    const Vector2 RoomScreenOffset(0, 3);

    std::shared_ptr<const Sprite> MakeTileSprite(char glyph, Color color)
    {
        std::vector<SpriteCell> cells
        {
            SpriteCell(glyph, (WORD)color, false),
        };

        return std::make_shared<const Sprite>(Vector2::One, std::move(cells));
    }

    TileSpriteCatalog CreateTileSprites()
    {
        TileSpriteCatalog catalog;
        catalog.Add(
            0x02,
            MakeTileSprite('.', Color::White)
        );

        // 원래는 빈 검은 공간이지만 구분을 위해
        catalog.Add(
            0x12,
            MakeTileSprite(' ', Color::White)
        );

        catalog.Add(
            0x14,
            MakeTileSprite('=', Color::Blue)
        );

        catalog.Add(
            0x28,
            MakeTileSprite('#', Color::Yellow)
        );

        catalog.Add(
            0x29,
            MakeTileSprite('#', Color::Yellow)
        );

        catalog.Add(
            0x2A,
            MakeTileSprite('#', Color::Yellow)
        );

        catalog.Add(
            0x3C,
            MakeTileSprite('#', Color::Yellow)
        );

        catalog.Add(
            0x3D,
            MakeTileSprite('#', Color::Yellow)
        );

        catalog.Add(
            0x3E,
            MakeTileSprite('#', Color::Yellow)
        );

        catalog.Add(
            0x30,
            MakeTileSprite('T', Color::Green)
        );

        catalog.Add(
            0x42,
            MakeTileSprite('T', Color::Green)
        );

        catalog.Add(
            0x43,
            MakeTileSprite('T', Color::Green)
        );

        catalog.Add(
            0x44,
            MakeTileSprite('T', Color::Green)
        );

        catalog.Add(
            0x50,
            MakeTileSprite('~', Color::Blue)
        );

        catalog.Add(
            0x51,
            MakeTileSprite('~', Color::Blue)
        );

        catalog.Add(
            0x52,
            MakeTileSprite('~', Color::Blue)
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

    if (!_loaded || _player)
    {
        return;
    }

    // 월드 좌표
    _player = SpawnActor<Player>(GetRoomWorldOrigin(_currentRoom) + Vector2(7, 2) * MapTileSize);
}

void OverworldLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (!_player) return;

    Vector2 direction = Vector2::Zero;

    if (Input::Get().GetKey(VK_UP)) direction = Vector2::Up;
    else if (Input::Get().GetKey(VK_DOWN)) direction = Vector2::Up * -1;
    else if (Input::Get().GetKey(VK_LEFT)) direction = Vector2::Right * -1;
    else if (Input::Get().GetKey(VK_RIGHT)) direction = Vector2::Right;

    // TODO: Facing 전환

    if (direction == Vector2::Zero) return;

    const int moveSteps = _player->ConsumeMoveSteps(deltaTime);
    for (int i = 0; i < moveSteps; ++i)
    {
        const Vector2 candidate = _player->GetWorldPosition() + direction;
        
        if (!CanMove(candidate))
        {
            _player->ClearMoveRemainder();
            break;
        }

        const RoomCoordinate nextRoom = GetRoomCoordinate(candidate);
        if (nextRoom != _currentRoom)
        {
            std::string error;
            if (!LoadRoom(nextRoom, error))
            {
                _player->ClearMoveRemainder();
                break;
            }
        }

        _player->MoveBy(direction);
    }
}

void OverworldLevel::Draw()
{
    Renderer& renderer = Renderer::Get();

    const Vector2 roomWorldOrigin = GetRoomWorldOrigin(_currentRoom);
    renderer.SetView(roomWorldOrigin, RoomScreenOffset);

    if (_roomSprite)
    {
        renderer.SubmitWorld(
            _roomSprite, 
            roomWorldOrigin, 
            0
        );
    }

    Level::Draw();

    renderer.Submit("[Overworld Level]", Vector2::Zero);
}

void OverworldLevel::LoadMap()
{
    const FilePath basePath = "../Content/Z1/Maps/Overworld";
    std::string error;

    if(!_loader.Load(basePath / "TileMap.txt", basePath / "BlockingMap.txt", error))
    {
        _loaded = false;
        return;
    }

    _tileSprites = CreateTileSprites();
    
    if (!LoadRoom({7, 7}, error))
    {
        _loaded = false;
        return;
    }

    _loaded = true;
}

bool OverworldLevel::LoadRoom(RoomCoordinate room, std::string& error)
{
    auto data = _loader.ExtractRoom(room, error);
    if (!data)
    {
        return false;
    }

    _roomData = std::move(data);
    _currentRoom = room;

    BuildRoomSprite();
    return true;
}

/*
* Room은 가로 16 x 세로 11 Sprite를 가짐
* Renderer가 Sprite에 Scale을 적용해서 확장하는 구조
*/
void OverworldLevel::BuildRoomSprite()
{
    if (!_roomData.has_value())
    {
        _roomSprite.reset();
        return;
    }

    // 렌더러가 1:1로만 그리게 변경되어 Room Sprite를 처음부터 (16, 11)의 (5, 3)배 한 걸로 만들어야 함
    const Vector2 spriteSize(RoomData::Width * MapTileSize.x, RoomData::Height * MapTileSize.y);
    std::vector<SpriteCell> tilemap(spriteSize.x * spriteSize.y, SpriteCell());

    // Room에 속하는 Tile 순회
    for (int tileY = 0; tileY < RoomData::Height; ++tileY)
    {
        for (int tileX = 0; tileX < RoomData::Width; ++tileX)
        {
            const TileId id = _roomData->GetTileId(tileX, tileY);
            std::shared_ptr<const Sprite> sprite;
            SpriteCell visual{ '?', (WORD)Color::White, false };

            if (_tileSprites.TryGet(id, sprite) && sprite)
            {
                visual = sprite->GetCell(0, 0);
            }

            // 타일 크기만큼 반복해서 그리기
            for (int offsetY = 0; offsetY < MapTileSize.y; ++offsetY)
            {
                for (int offsetX = 0; offsetX < MapTileSize.x; ++offsetX)
                {
                    const int destX = tileX * MapTileSize.x + offsetX;
                    const int destY = tileY * MapTileSize.y + offsetY;
                    const int destIndex = destY * spriteSize.x + destX;

                    tilemap[destIndex] = visual;
                }
            }
        }
    }

    _roomSprite = std::make_shared<const Sprite>(spriteSize, std::move(tilemap));
}

/// <summary>
/// 현재 위치(전체 맵에 Cell 좌표 기준)가 어디 Room에 속하는지
/// </summary>
/// <param name="mapCellPosition"></param>
/// <returns></returns>
RoomCoordinate OverworldLevel::GetRoomCoordinate(const Craft::Vector2& worldPosition) const
{
    assert(worldPosition.x >= 0 && worldPosition.y >= 0);

    // 룸의 셀 단위 가로세로길이
    const int roomCellWidth = RoomData::Width * MapTileSize.x;
    const int roomCellHeight = RoomData::Height * MapTileSize.y;

    return RoomCoordinate(worldPosition.x / roomCellWidth, worldPosition.y / roomCellHeight);
}

// Map 기준 Room의 좌상단 셀 좌표
Vector2 OverworldLevel::GetRoomWorldOrigin(RoomCoordinate room) const
{
    return Vector2
    {
        room.x * RoomData::Width * MapTileSize.x,
        room.y * RoomData::Height * MapTileSize.y
    };
}

bool OverworldLevel::CanMove(const Vector2& candidate) const
{
    if (!_player || !_loader.IsLoaded())
    {
        return false;
    }

    const auto& box = _player->GetComponent<BoxComponent>();
    if (!box)
    {
        return false;
    }

    const Vector2 boxPosition = candidate + box->GetOffset();
    return _loader.CanOccupyWorldRect(boxPosition, box->GetSize(), MapTileSize);
}
