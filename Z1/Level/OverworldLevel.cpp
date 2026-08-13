#include "pch.h"
#include "OverworldLevel.h"
#include <Actor/Player.h>
#include <Core/Input.h>
#include <Render/Renderer.h>
#include <Render/Sprite.h>
#include <Component/BoxComponent.h>
#include <filesystem>
#include <Engine/Engine.h>
#include <Actor/SwordAttack.h>
#include <Actor/Enemy.h>
#include <Math/MathUtility.h>
#include <set>

using namespace Craft;
using FilePath = std::filesystem::path;

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
    const Vector2 MapTileSize(10, 5);
    const Vector2 RoomScreenOffset(0, 3);

    const Vector2 EnemyBoxSize(1, 1);

    std::shared_ptr<const Sprite> MakeTileSprite(char glyph, Color color)
    {
        std::vector<SpriteCell> cells
        {
            SpriteCell(glyph, (WORD)color, false),
        };

        return std::make_shared<const Sprite>(Vector2::One, std::move(cells));
    }

    const TileSpriteMap& CreateTileSprites()
    {
        static const TileSpriteMap sprites
        {
            {0x02, MakeTileSprite('.', Color::White)},
            {0x12, MakeTileSprite(' ', Color::White)},
            {0x14, MakeTileSprite('=', Color::Blue)},
            {0x28, MakeTileSprite('#', Color::Yellow)},
            {0x29, MakeTileSprite('#', Color::Yellow)},
            {0x2A, MakeTileSprite('#', Color::Yellow)},
            {0x3C, MakeTileSprite('#', Color::Yellow)},
            {0x3D, MakeTileSprite('#', Color::Yellow)},
            {0x3E, MakeTileSprite('#', Color::Yellow)},
            {0x30, MakeTileSprite('T', Color::Green)},
            {0x42, MakeTileSprite('T', Color::Green)},
            {0x43, MakeTileSprite('T', Color::Green)},
            {0x44, MakeTileSprite('T', Color::Green)},
            {0x50, MakeTileSprite('~', Color::Blue)},
            {0x51, MakeTileSprite('~', Color::Blue)},
            {0x52, MakeTileSprite('~', Color::Blue)},
        };

        return sprites;
    }

    bool Overlaps(const Vector2& lhsPos, const Vector2& lhsSize, const Vector2& rhsPos, const Vector2& rhsSize)
    {
        const int lhsRight  = lhsPos.x + lhsSize.x - 1;
        const int lhsBottom = lhsPos.y + lhsSize.y - 1;
        const int rhsRight  = rhsPos.x + rhsSize.x - 1;
        const int rhsBottom = rhsPos.y + rhsSize.y - 1;

        return !(lhsRight < rhsPos.x || rhsRight < lhsPos.x || lhsBottom < rhsPos.y || rhsBottom < lhsPos.y);
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
    _player = SpawnActor<Player>(GetRoomWorldOrigin(_currentRoom) + Vector2(7, 2) * MapTileSize, 6);

    if (!_bgmStarted)
    {
        Engine::Get().PlayBGM("Z1/02. Overworld of Hyrule.wav");
        _bgmStarted = true;
    }
}

void OverworldLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    // TODO: 나중에 PlayerContoller 같은 걸로 다 이관시켜야 하나?
    if (!_player) return;

    const bool playerWasKnockback = UpdatePawnKnockback(*_player, deltaTime);
    if (!playerWasKnockback)
    {
        const Vector2 movementInputDirection = _player->GetMovementInputDirection();
        if (movementInputDirection != Vector2::Zero)
        {
            _player->CancelAttack();    // 이동 시 공격 중단
            UpdatePlayerMovement(deltaTime, movementInputDirection);
        }
        else if (_player->HasSword() && Input::Get().GetKeyDown(VK_SPACE) && !_player->IsAttacking())
        {
            Vector2 offset = Vector2::Zero;
            switch (_player->GetFacing())
            {
            case Facing::Up:    offset = Vector2::Up; break;
            case Facing::Down:  offset = Vector2::Up * -1; break;
            case Facing::Left:  offset = Vector2::Right * -1; break;
            case Facing::Right: offset = Vector2::Right; break;
            default:break;
            }

            auto attack = SpawnActor<SwordAttack>(offset, _player, 1);
            attack->AttachTo(_player, false);
            _player->SetActiveAttack(attack);
        }
    }

    UpdateEnemyMovement(deltaTime);
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

void OverworldLevel::EndPlay()
{
    Level::EndPlay();

    Engine::Get().StopBGM();
    _bgmStarted = false;
}

bool OverworldLevel::LoadMap()
{
    const FilePath basePath = "../Content/Z1/Maps/Overworld";
    std::string error;

    if(!_map.Load(basePath / "TileMap.txt", basePath / "BlockingMap.txt", error))
    {
        _loaded = false;
        return false;
    }

    _tileSprites = CreateTileSprites();
    _currentRoom = StartRoom;
    BuildRoomSprite();

    _loaded = true;
    return true;
}

bool OverworldLevel::TryChangeRoom(RoomCoordinate room)
{
    if (room == _currentRoom)
    {
        return false;
    }

    DestroyRoomEnemies();   // 원래 Room 적 날리고

    _currentRoom = room;
    BuildRoomSprite();

    SpawnRoomEnemies();     // 새로운 Room 적 생성하고
    
    return true;
}

/*
* Room은 가로 16 x 세로 11 Sprite를 가짐
* Renderer가 Sprite에 Scale을 적용해서 확장하는 구조
*/
void OverworldLevel::BuildRoomSprite()
{
    // 렌더러가 1:1로만 그리게 변경되어 Room Sprite를 처음부터 (16, 11)의 (5, 3)배 한 걸로 만들어야 함
    const int roomTileWidth  = RoomTileWidth;
    const int roomTileHeight = RoomTileHeight;

    const Vector2 spriteSize(roomTileWidth * MapTileSize.x, roomTileHeight * MapTileSize.y);
    std::vector<SpriteCell> roomCells(spriteSize.x * spriteSize.y, SpriteCell());

    // 현재 Room의 Map 기준 (x, y) 좌표
    const int mapTileOriginX = _currentRoom.x * roomTileWidth;
    const int mapTileOriginY = _currentRoom.y * roomTileHeight;

    // Room에 속하는 Tile 순회
    for (int roomTileY = 0; roomTileY < roomTileHeight; ++roomTileY)
    {
        for (int roomTileX = 0; roomTileX < roomTileWidth; ++roomTileX)
        {
            const int mapTileX = mapTileOriginX + roomTileX;
            const int mapTileY = mapTileOriginY + roomTileY;
            const TileId id = _map.GetTileId(mapTileX, mapTileY);

            std::shared_ptr<const Sprite> sprite;
            SpriteCell visual{ '?', (WORD)Color::White, false };
            
            const auto it = _tileSprites.find(id);
            if (it != _tileSprites.end() && it->second)
            {
                visual = it->second->GetCell(0, 0);
            }

            // 타일 크기만큼 반복해서 그리기
            for (int offsetY = 0; offsetY < MapTileSize.y; ++offsetY)
            {
                for (int offsetX = 0; offsetX < MapTileSize.x; ++offsetX)
                {
                    const int destX = roomTileX * MapTileSize.x + offsetX;
                    const int destY = roomTileY * MapTileSize.y + offsetY;
                    const int destIndex = destY * spriteSize.x + destX;

                    roomCells[destIndex] = visual;
                }
            }
        }
    }

    _roomSprite = std::make_shared<const Sprite>(spriteSize, std::move(roomCells));
}

/// <summary>
/// 현재 위치(전체 맵에 Cell 좌표 기준)가 어디 Room에 속하는지
/// </summary>
/// <param name="mapCellPosition"></param>
/// <returns></returns>
RoomCoordinate OverworldLevel::GetRoomCoordinate(const Vector2& worldPosition) const
{
    assert(worldPosition.x >= 0 && worldPosition.y >= 0);

    // Room의 셀 단위 가로/세로 길이
    const int roomCellWidth =  RoomTileWidth * MapTileSize.x;
    const int roomCellHeight = RoomTileHeight * MapTileSize.y;

    return RoomCoordinate(worldPosition.x / roomCellWidth, worldPosition.y / roomCellHeight);
}

// Map 기준 Room의 좌상단 셀 좌표
Vector2 OverworldLevel::GetRoomWorldOrigin(RoomCoordinate room) const
{
    return Vector2
    {
        room.x * RoomTileWidth * MapTileSize.x,
        room.y * RoomTileHeight * MapTileSize.y
    };
}

bool OverworldLevel::CanMoveTo(const Craft::Vector2& destination, const Pawn& mover)
{
    const auto& moverBox = mover.GetComponent<BoxComponent>();
    if (!moverBox)
    {
        return false;
    }

    const Vector2 moverPosition = destination + moverBox->GetOffset();
    if (!_map.CanOccupyWorldRect(moverPosition, moverBox->GetSize(), MapTileSize))
    {
        return false;
    }

    auto IsOverlapping = [&mover, &moverPosition, &moverBox](const std::shared_ptr<Pawn>& other) -> bool
        {
            if (!other || !other->IsActive() || other.get() == &mover)
            {
                return false;
            }

            const auto& otherBox = other->GetComponent<BoxComponent>();
            if (!otherBox)
            {
                return false;
            }

            const Vector2 otherPosition = other->GetWorldPosition() + otherBox->GetOffset();

            return Overlaps(moverPosition, moverBox->GetSize(), otherPosition, otherBox->GetSize());
        };

    if (IsOverlapping(_player))
    {
        return false;
    }

    if (mover.IsA<Enemy>())
    {
        return true;
    }

    for (const auto& enemy : _roomEnemies)
    {
        if (IsOverlapping(enemy))
        {
            return false;
        }
    }

    return true;
}

bool OverworldLevel::UpdatePawnKnockback(Pawn& pawn, float deltaTime)
{
    if (!pawn.IsKnockback())
    {
        return false;
    }

    const Vector2 direction = pawn.GetKnockbackDirection();
    const int steps = pawn.ConsumeKnockbackSteps(deltaTime);

    for (int i = 0; i < steps; ++i)
    {
        const Vector2 destination = pawn.GetWorldPosition() + direction;
        if (!CanMoveTo(destination, pawn))
        {
            pawn.StopKnockback();
            break;
        }

        pawn.MoveBy(direction);

        if (pawn.IsA<Player>())
        {
            TryChangeRoom(GetRoomCoordinate(pawn.GetWorldPosition()));
        }
    }

    return true;
}

void OverworldLevel::SpawnRoomEnemies()
{
    if (!_player || _currentRoom == StartRoom)
    {
        return;
    }

    const auto spawnPlan = _enemySpawner.BuildSpawnPlan(
        _currentRoom,
        _map,
        _player->GetWorldPosition(),
        MapTileSize,
        _worldSeed,
        GetRoomWorldOrigin(_currentRoom)
    );

    for (const auto& spawn : spawnPlan)
    {
        _roomEnemies.push_back(SpawnActor<Enemy>(spawn.worldPosition, spawn.maxHp));
    }
}

void OverworldLevel::DestroyRoomEnemies()
{
    for (const auto& enemy : _roomEnemies)
    {
        if (enemy)
        {
            enemy->Destroy();
        }
    }

    _roomEnemies.clear();
}

void OverworldLevel::UpdatePlayerMovement(float deltaTime, const Vector2& delta)
{
    const int moveSteps = _player->ConsumeMoveSteps(deltaTime);
    for (int i = 0; i < moveSteps; ++i)
    {
        const Vector2 candidate = _player->GetWorldPosition() + delta;

        if (!CanMoveTo(candidate, *_player))
        {
            _player->ClearMoveRemainder();
            break;
        }

        const RoomCoordinate nextRoom = GetRoomCoordinate(candidate);
        _player->MoveBy(delta); // 적 Spawn할 때 Player 유무를 검사하기 때문에, 먼저 이동시킴
        TryChangeRoom(nextRoom);
    }
}

void OverworldLevel::UpdateEnemyMovement(float deltaTime)
{
    for (const auto& enemy : _roomEnemies)
    {
        if (!enemy || !enemy->IsActive())
        {
            continue;
        }

        if (UpdatePawnKnockback(*enemy, deltaTime))
        {
            continue;
        }

        const int moveSteps = enemy->ConsumeMoveSteps(deltaTime);
        for (int i = 0; i < moveSteps; ++i)
        {
            const Vector2 delta = enemy->GetChaseDelta(_player->GetWorldPosition());
            const Vector2 candidate = enemy->GetWorldPosition() + delta;

            if (!CanMoveTo(candidate, *enemy))
            {
                enemy->ClearMoveRemainder();
                break;
            }

            enemy->MoveBy(delta);
        }
    }
}
