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
#include <Actor/Projectile.h>
#include <Game/Game.h>

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

    constexpr TileId EntryMarkerTileId = 0x12;
    constexpr int StartSwordCaveLocalX = 4; // 스타팅 룸에서 동굴 입구 논리 좌표
    constexpr int StartSwordCaveLocalY = 1;

    constexpr RoomCoordinate Dungeon1EntranceRoom{ 7, 3 };
    constexpr int Dungeon1EntranceLocalX = 7;
    constexpr int Dungeon1EntranceLocalY = 4;

    bool IsAccessibleRoom(RoomCoordinate room)
    {
        return room == RoomCoordinate{ 7, 7 } ||
               room == RoomCoordinate{ 7, 6 } ||
               room == RoomCoordinate{ 8, 6 } ||
               room == RoomCoordinate{ 8, 5 } ||
               room == RoomCoordinate{ 8, 4 } ||
               room == RoomCoordinate{ 8, 3 } ||
               room == Dungeon1EntranceRoom;
    }

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
            // 0x00 ~ 0x11: 갈색/회색 절벽·나무·평지
            {0x00, MakeTileSprite('#', Color::DarkYellow)},
            {0x01, MakeTileSprite('#', Color::DarkYellow)},
            {0x02, MakeTileSprite('.', Color::DarkYellow)},
            {0x03, MakeTileSprite('#', Color::DarkYellow)},
            {0x04, MakeTileSprite('#', Color::DarkYellow)},
            {0x05, MakeTileSprite('#', Color::DarkYellow)},
            {0x06, MakeTileSprite('T', Color::Green)},
            {0x07, MakeTileSprite('T', Color::Green)},
            {0x08, MakeTileSprite('#', Color::Gray)},
            {0x09, MakeTileSprite('T', Color::Green)},
            {0x0A, MakeTileSprite('T', Color::Green)},
            {0x0B, MakeTileSprite('T', Color::Green)},
            {0x0C, MakeTileSprite('#', Color::Gray)},
            {0x0D, MakeTileSprite('#', Color::Gray)},
            {0x0E, MakeTileSprite('.', Color::Gray)},
            {0x0F, MakeTileSprite('#', Color::Gray)},
            {0x10, MakeTileSprite('#', Color::Gray)},
            {0x11, MakeTileSprite('#', Color::Gray)},
            {0x12, MakeTileSprite(' ', Color::Black)},  // 예외: 빈 공간

            // 0x14 ~ 0x25: 다리, 나무, 절벽, 특수 구조물
            {0x14, MakeTileSprite('=', Color::Blue)},
            {0x15, MakeTileSprite('T', Color::DarkRed)},
            {0x16, MakeTileSprite('T', Color::DarkRed)},
            {0x17, MakeTileSprite('#', Color::DarkYellow)},
            {0x18, MakeTileSprite('#', Color::DarkYellow)},
            {0x19, MakeTileSprite('#', Color::DarkYellow)},
            {0x1A, MakeTileSprite('=', Color::Green)},
            {0x1B, MakeTileSprite('T', Color::Green)},
            {0x1C, MakeTileSprite('T', Color::Green)},
            {0x1D, MakeTileSprite('T', Color::Green)},
            {0x1E, MakeTileSprite('O', Color::Green)},
            {0x1F, MakeTileSprite('T', Color::Green)},
            {0x20, MakeTileSprite('=', Color::Blue)},
            {0x21, MakeTileSprite('+', Color::White)},
            {0x22, MakeTileSprite('O', Color::White)},
            {0x23, MakeTileSprite('.', Color::Gray)},
            {0x24, MakeTileSprite('#', Color::Gray)},
            {0x25, MakeTileSprite('#', Color::White)},

            // 0x28 ~ 0x39: 산맥, 숲, 동굴/구조물
            {0x28, MakeTileSprite('#', Color::DarkYellow)},
            {0x29, MakeTileSprite('#', Color::DarkYellow)},
            {0x2A, MakeTileSprite('#', Color::DarkYellow)},
            {0x2B, MakeTileSprite('#', Color::DarkYellow)},
            {0x2C, MakeTileSprite('O', Color::DarkRed)},
            {0x2D, MakeTileSprite('#', Color::DarkYellow)},
            {0x2E, MakeTileSprite('T', Color::Green)},
            {0x2F, MakeTileSprite('T', Color::Green)},
            {0x30, MakeTileSprite('T', Color::Green)},
            {0x31, MakeTileSprite('T', Color::Green)},
            {0x32, MakeTileSprite('O', Color::Green)},
            {0x33, MakeTileSprite('T', Color::Green)},
            {0x34, MakeTileSprite('#', Color::Gray)},
            {0x35, MakeTileSprite('#', Color::Gray)},
            {0x36, MakeTileSprite('#', Color::Gray)},
            {0x37, MakeTileSprite('#', Color::Gray)},
            {0x38, MakeTileSprite('O', Color::White)},
            {0x39, MakeTileSprite('#', Color::Gray)},

            // 0x3C ~ 0x4D: 산맥, 숲, 물가
            {0x3C, MakeTileSprite('#', Color::DarkYellow)},
            {0x3D, MakeTileSprite('#', Color::DarkYellow)},
            {0x3E, MakeTileSprite('#', Color::DarkYellow)},
            {0x3F, MakeTileSprite('=', Color::DarkRed)},
            {0x40, MakeTileSprite('.', Color::DarkYellow)},
            {0x41, MakeTileSprite('=', Color::DarkRed)},
            {0x42, MakeTileSprite('T', Color::Green)},
            {0x43, MakeTileSprite('T', Color::Green)},
            {0x44, MakeTileSprite('T', Color::Green)},
            {0x45, MakeTileSprite('T', Color::Green)},
            {0x46, MakeTileSprite('~', Color::Blue)},
            {0x47, MakeTileSprite('T', Color::Green)},
            {0x48, MakeTileSprite('#', Color::Gray)},
            {0x49, MakeTileSprite('#', Color::Gray)},
            {0x4A, MakeTileSprite('#', Color::Gray)},
            {0x4B, MakeTileSprite('#', Color::Gray)},
            {0x4C, MakeTileSprite('=', Color::Blue)},
            {0x4D, MakeTileSprite('.', Color::Gray)},

            // 0x50 ~ 0x61: 팔레트별 평지·물·바위 지형
            {0x50, MakeTileSprite('~', Color::Blue)},
            {0x51, MakeTileSprite('~', Color::Blue)},
            {0x52, MakeTileSprite('~', Color::Blue)},
            {0x53, MakeTileSprite('.', Color::DarkYellow)},
            {0x54, MakeTileSprite('.', Color::DarkYellow)},
            {0x55, MakeTileSprite('.', Color::DarkYellow)},
            {0x56, MakeTileSprite('~', Color::Blue)},
            {0x57, MakeTileSprite('~', Color::Blue)},
            {0x58, MakeTileSprite('~', Color::Blue)},
            {0x59, MakeTileSprite('.', Color::DarkYellow)},
            {0x5A, MakeTileSprite('.', Color::DarkYellow)},
            {0x5B, MakeTileSprite('.', Color::DarkYellow)},
            {0x5C, MakeTileSprite('~', Color::Blue)},
            {0x5D, MakeTileSprite('~', Color::Blue)},
            {0x5E, MakeTileSprite('~', Color::Blue)},
            {0x5F, MakeTileSprite('.', Color::Gray)},
            {0x60, MakeTileSprite('.', Color::Gray)},
            {0x61, MakeTileSprite('.', Color::Gray)},

            // 0x64 ~ 0x75: 위 지형의 하단/연결 조각
            {0x64, MakeTileSprite('~', Color::Blue)},
            {0x65, MakeTileSprite('~', Color::Blue)},
            {0x66, MakeTileSprite('~', Color::Blue)},
            {0x67, MakeTileSprite('.', Color::DarkYellow)},
            {0x68, MakeTileSprite('.', Color::DarkYellow)},
            {0x69, MakeTileSprite('.', Color::DarkYellow)},
            {0x6A, MakeTileSprite('~', Color::Blue)},
            {0x6B, MakeTileSprite('~', Color::Blue)},
            {0x6C, MakeTileSprite('~', Color::Blue)},
            {0x6D, MakeTileSprite('.', Color::DarkYellow)},
            {0x6E, MakeTileSprite('.', Color::DarkYellow)},
            {0x6F, MakeTileSprite('.', Color::DarkYellow)},
            {0x70, MakeTileSprite('~', Color::Blue)},
            {0x71, MakeTileSprite('~', Color::Blue)},
            {0x72, MakeTileSprite('~', Color::Blue)},
            {0x73, MakeTileSprite('.', Color::Gray)},
            {0x74, MakeTileSprite('.', Color::Gray)},
            {0x75, MakeTileSprite('.', Color::Gray)},

            // 0x78 ~ 0x89: 위 지형의 하단/연결 조각
            {0x78, MakeTileSprite('~', Color::Blue)},
            {0x79, MakeTileSprite('~', Color::Blue)},
            {0x7A, MakeTileSprite('~', Color::Blue)},
            {0x7B, MakeTileSprite('.', Color::DarkYellow)},
            {0x7C, MakeTileSprite('.', Color::DarkYellow)},
            {0x7D, MakeTileSprite('.', Color::DarkYellow)},
            {0x7E, MakeTileSprite('~', Color::Blue)},
            {0x7F, MakeTileSprite('~', Color::Blue)},
            {0x80, MakeTileSprite('~', Color::Blue)},
            {0x81, MakeTileSprite('.', Color::DarkYellow)},
            {0x82, MakeTileSprite('.', Color::DarkYellow)},
            {0x83, MakeTileSprite('.', Color::DarkYellow)},
            {0x84, MakeTileSprite('~', Color::Blue)},
            {0x85, MakeTileSprite('~', Color::Blue)},
            {0x86, MakeTileSprite('~', Color::Blue)},
            {0x87, MakeTileSprite('.', Color::Gray)},
            {0x88, MakeTileSprite('.', Color::Gray)},
            {0x89, MakeTileSprite('.', Color::Gray)},

            // 0x8C ~ 0x9D: 물가 모서리, 다리, 동굴/구조물
            {0x8C, MakeTileSprite('.', Color::DarkYellow)},
            {0x8D, MakeTileSprite('.', Color::DarkYellow)},
            {0x8E, MakeTileSprite('.', Color::DarkYellow)},
            {0x8F, MakeTileSprite('.', Color::DarkYellow)},
            {0x90, MakeTileSprite('~', Color::Blue)},
            {0x91, MakeTileSprite('=', Color::DarkRed)},
            {0x92, MakeTileSprite('.', Color::DarkYellow)},
            {0x93, MakeTileSprite('.', Color::DarkYellow)},
            {0x94, MakeTileSprite('.', Color::DarkYellow)},
            {0x95, MakeTileSprite('.', Color::DarkYellow)},
            {0x96, MakeTileSprite('O', Color::White)},
            {0x97, MakeTileSprite('=', Color::Green)},
            {0x98, MakeTileSprite('.', Color::Gray)},
            {0x99, MakeTileSprite('.', Color::Gray)},
            {0x9A, MakeTileSprite('.', Color::Gray)},
            {0x9B, MakeTileSprite('.', Color::Gray)},
            {0x9C, MakeTileSprite('O', Color::DarkRed)},
            {0x9D, MakeTileSprite('O', Color::White)},
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

    bool IsInContact(const Pawn& lhs, const Pawn& rhs)
    {
        const std::shared_ptr<BoxComponent>& lhsBox = lhs.GetComponent<BoxComponent>();
        const std::shared_ptr<BoxComponent>& rhsBox = rhs.GetComponent<BoxComponent>();

        if (!lhsBox || !rhsBox) return false;

        const Vector2 lhsPos = lhs.GetWorldPosition() + lhsBox->GetOffset();
        const Vector2 rhsPos = rhs.GetWorldPosition() + rhsBox->GetOffset();
        const Vector2 lhsSize = lhsBox->GetSize();
        const Vector2 rhsSize = rhsBox->GetSize();

        const int lhsLeft     = lhsPos.x;
        const int lhsTop      = lhsPos.y;
        const int lhsRight    = lhsLeft + lhsSize.x - 1;
        const int lhsBottom   = lhsTop + lhsSize.y - 1;

        const int rhsLeft     = rhsPos.x;
        const int rhsTop      = rhsPos.y;
        const int rhsRight    = rhsLeft + rhsSize.x - 1;
        const int rhsBottom   = rhsTop + rhsSize.y - 1;

        const bool xOverlap = lhsLeft <= rhsRight && rhsLeft <= lhsRight;
        const bool yOverlap = lhsTop <= rhsBottom && rhsTop <= lhsBottom;
        const bool xAdjacent = lhsRight + 1 >= rhsLeft && rhsRight + 1 >= lhsLeft;
        const bool yAdjacent = lhsBottom + 1 >= rhsTop && rhsBottom + 1 >= lhsTop;

        return xAdjacent && yOverlap || yAdjacent && xOverlap;
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

    if (!_loaded)
    {
        return;
    }

    Game& game = dynamic_cast<Game&>(Engine::Get());

    if (!_bgmStarted)
    {
        Engine::Get().PlayBGM("Z1/02. Overworld of Hyrule.wav");
        _bgmStarted = true;
    }

    if (_player)
    {
        if (_needsPlayerSync)
        {
            _player->SetHealth(game.GetPlayerHp());
            _needsPlayerSync = false;
        }

        if (game.HasSword())
        {
            _player->EquipSword();
        }

        return;
    }

    // 월드 좌표
    _player = SpawnActor<Player>(
        GetRoomWorldOrigin(_currentRoom) + Vector2(7, 2) * MapTileSize,
        Game::PlayerMaxHp
    );
    _player->SetHealth(game.GetPlayerHp());
    _needsPlayerSync = false;
}

void OverworldLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    // TODO: 나중에 PlayerContoller 같은 걸로 다 이관시켜야 하나?
    if (!_player) return;

    if (_player->IsDead())
    {
        if (!_gameOverPending)
        {
            _gameOverPending = true;
            _gameOverTimer.Reset();
            return;
        }

        _gameOverTimer.Tick(deltaTime);
        if (_gameOverTimer.TimeOver())
        {
            Game& game = dynamic_cast<Game&>(Engine::Get());
            game.ChangeLevel(State::GameOver);
        }

        return;
    }

    const bool playerWasKnockback = UpdatePawnKnockback(*_player, deltaTime);
    if (!playerWasKnockback)
    {
        const Vector2 movementInputDirection = _player->GetMovementInputDirection();
        if (movementInputDirection != Vector2::Zero)
        {
            _player->CancelAttack();    // 이동 시 공격 중단
            UpdatePlayerMovement(deltaTime, movementInputDirection);
        }
        else if (_player->HasSword() && Input::Get().GetKeyDown('A') && !_player->IsAttacking())
        {
            Vector2 direction = _player->GetFacingDirection();

            auto attack = SpawnActor<SwordAttack>(direction, _player, 1);
            attack->AttachTo(_player, false);
            _player->SetActiveAttack(attack);

            const bool canShootSwordBeam = _player->IsFullHp();
            Engine::Get().PlayOneShot(
                canShootSwordBeam
                ? "Z1/LOZ_Sword_Combined.wav"
                : "Z1/LOZ_Sword_Slash.wav"
            );

            if (canShootSwordBeam)
            {
                ProjectileSpec swordBeam
                {
                    .type = ProjectileType::SwordBeam,
                    .faction = ProjectileFaction::Player,
                    .damage = 1,
                    .speed = 40.f,
                    .lifetime = 1.5f,
                    .direction = direction,
                    .boxSize = Vector2::One,
                    .image = "*",
                    .color = Color::White,
                    .sortingOrder = 11
                };

                SpawnProjectile(
                    GetProjectileSpawnPosition(*_player, swordBeam),
                    swordBeam,
                    _player
                );
            }
        }
    }

    UpdateEnemyMovement(deltaTime);
    TakeContactDamageToPlayer();
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

    renderer.Submit("[Overworld]", Vector2(2, 1));

    if (_player)
    {
        const std::string hp = "[HP " + std::to_string(_player->GetHp()) + "/" + std::to_string(_player->GetMaxHp()) + "]";
        renderer.Submit(hp, Vector2(16, 1));

        const std::string sword = _player->HasSword() ? "[SWORD]" : "[NO SWORD]";
        renderer.Submit(sword, Vector2(30, 1));
    }
}

void OverworldLevel::EndPlay()
{
    Level::EndPlay();

    if (_player)
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.SetPlayerHp(_player->GetHp());
    }

    _needsPlayerSync = true;

    Engine::Get().StopBGM();
    _bgmStarted = false;
}

bool OverworldLevel::CanProjectileOccupy(Craft::Vector2 destination, const Projectile& projectile)
{
    const auto box = projectile.GetComponent<BoxComponent>();
    if (!box)
    {
        return false;
    }

    const Vector2 boxPos = destination + box->GetOffset();
    if (!IsInsideCurrentRoom(boxPos, box->GetSize()))
    {
        return false;
    }

    return _map.CanOccupyWorldRect(boxPos, box->GetSize(), MapTileSize);
}

std::shared_ptr<Projectile> OverworldLevel::SpawnProjectile(Craft::Vector2 position, const ProjectileSpec& spec, const std::shared_ptr<Pawn>& instigator)
{
    auto projectile = SpawnActor<Projectile>(position, spec, instigator);
    if (projectile)
    {
        _roomProjectiles.emplace_back(projectile);
    }

    return projectile;
}

std::shared_ptr<Enemy> OverworldLevel::SpawnEnemy(const EnemySpawnData& spawn)
{
    // SpawnActor<Octorok>, ... maxHp는 제거됨
    switch (spawn.kind)
    {
    case EnemyKind::Octorok: return SpawnActor<Octorok>(spawn.worldPosition, spawn.variant);
    case EnemyKind::Moblin:
    default:
        return nullptr;
    }
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
    DestroyRoomProjectiles();

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

    const Vector2 spriteSize(RoomTileWidth * MapTileSize.x, RoomTileHeight * MapTileSize.y);
    std::vector<SpriteCell> roomCells(spriteSize.x * spriteSize.y, SpriteCell());

    // 현재 Room의 Map 기준 (x, y) 좌표
    const int mapTileOriginX = _currentRoom.x * RoomTileWidth;
    const int mapTileOriginY = _currentRoom.y * RoomTileHeight;

    // Room에 속하는 Tile 순회
    for (int roomTileY = 0; roomTileY < RoomTileHeight; ++roomTileY)
    {
        for (int roomTileX = 0; roomTileX < RoomTileWidth; ++roomTileX)
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

RoomCoordinate OverworldLevel::GetRoomCoordinateAtLeadingEdge(
    const Vector2& destination,
    const Pawn& pawn,
    const Vector2& direction) const
{
    const auto box = pawn.GetComponent<BoxComponent>();
    if (!box)
    {
        return GetRoomCoordinate(destination);
    }

    Vector2 probePosition = destination + box->GetOffset();
    const Vector2 boxSize = box->GetSize();

    if (direction.x > 0)
    {
        probePosition.x += boxSize.x - 1;
    }
    else if (direction.y > 0)
    {
        probePosition.y += boxSize.y - 1;
    }

    return GetRoomCoordinate(probePosition);
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
    if (mover.IsA<Enemy>())
    {
        if (!IsInsideCurrentRoom(moverPosition, moverBox->GetSize()))
        {
            return false;
        }
    }

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

        if (pawn.IsA<Player>() &&
            !IsAccessibleRoom(GetRoomCoordinateAtLeadingEdge(
                destination,
                pawn,
                direction)))
        {
            pawn.StopKnockback();
            break;
        }

        pawn.MoveBy(direction);

        if (pawn.IsA<Player>())
        {
            TryChangeRoom(GetRoomCoordinateAtLeadingEdge(
                pawn.GetWorldPosition(),
                pawn,
                direction
            ));
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
        if (auto enemy = SpawnEnemy(spawn))
        {
            _roomEnemies.emplace_back(enemy);
        }
        //_roomEnemies.push_back(SpawnActor<Enemy>(spawn.worldPosition, spawn.maxHp));
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
        if (TryEnterEntrance(candidate))    // 0x12가 기본적으로 Block이라 먼저 검사
        {
            _player->ClearMoveRemainder();
            break;
        }

        if (!CanMoveTo(candidate, *_player))
        {
            _player->ClearMoveRemainder();
            break;
        }

        const RoomCoordinate nextRoom = GetRoomCoordinateAtLeadingEdge(
            candidate,
            *_player,
            delta
        );

        if (!IsAccessibleRoom(nextRoom))
        {
            _player->ClearMoveRemainder();
            break;
        }

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

        enemy->Think(deltaTime, *_player);
        
        EnemyAttackRequest request;
        if (enemy->ConsumeAttackRequest(request))
        {
            for (const ProjectileSpec& projectile : request.projectiles)
            {
                SpawnProjectile(
                    GetProjectileSpawnPosition(*enemy, projectile),
                    projectile,
                    enemy
                );
            }
        }

        const Vector2 delta = enemy->GetDesiredMove();

        if (UpdatePawnKnockback(*enemy, deltaTime))
        {
            continue;
        }

        const int moveSteps = enemy->ConsumeMoveSteps(deltaTime);
        for (int i = 0; i < moveSteps; ++i)
        {
            //const Vector2 delta = enemy->GetChaseDelta(_player->GetWorldPosition());
            const Vector2 candidate = enemy->GetWorldPosition() + delta;

            if (!CanMoveTo(candidate, *enemy))
            {
                enemy->ClearMoveRemainder();
                enemy->OnMoveBlocked();
                break;
            }

            enemy->MoveBy(delta);
        }
    }
}

void OverworldLevel::DestroyRoomProjectiles()
{
    for (const auto& projectile : _roomProjectiles)
    {
        if (projectile)
        {
            projectile->Destroy();
        }
    }

    _roomProjectiles.clear();
}

bool OverworldLevel::IsInsideCurrentRoom(Craft::Vector2 boxPosition, Craft::Vector2 boxSize)
{
    if (boxSize.x <= 0 || boxSize.y <= 0) return false;

    const Vector2 origin = GetRoomWorldOrigin(_currentRoom);

    const Vector2 roomSize{ RoomTileWidth * MapTileSize.x, RoomTileHeight * MapTileSize.y };

    const int left = boxPosition.x;
    const int top = boxPosition.y;
    const int right = left + boxSize.x - 1;
    const int bottom = top + boxSize.y - 1;

    const int roomLeft = origin.x;
    const int roomTop = origin.y;
    const int roomRight = roomLeft + roomSize.x - 1;
    const int roomBottom = roomTop + roomSize.y - 1;

    return left >= roomLeft &&
        top >= roomTop &&
        right <= roomRight &&
        bottom <= roomBottom;
}

void OverworldLevel::TakeContactDamageToPlayer()
{
    if (!_player || _player->IsDead())
    {
        return;
    }

    for (const auto& enemy : _roomEnemies)
    {
        if (!enemy || !enemy->IsActive() || enemy->IsDead())
        {
            continue;
        }

        if (IsInContact(*_player, *enemy))
        {
            _player->TakeDamage(enemy->GetContactDamage(), enemy, enemy);
        }
    }
}

/*
* TileId가 0x12인지 검사
* Room 좌표 확인
* Room 내부 타일 좌표 확인
 * (7, 7) Room의 (4, 1) Tile이면 SwordCave
 * (7, 3) Room의 (7, 4) Tile이면 Dungeon1
*/
std::optional<EntranceType> OverworldLevel::ResolveEntrance(Vector2 destination, const Pawn& mover)
{
    auto box = mover.GetComponent<BoxComponent>();
    if (!box) return std::nullopt;

    Vector2 boxPos = destination + box->GetOffset();
    Vector2 boxSize = box->GetSize();

    int left = boxPos.x;
    int top = boxPos.y;
    int right = left + boxSize.x - 1;
    int bottom = top + boxSize.y - 1;

    int mapWidth = OverworldMap::Width * MapTileSize.x;
    int mapHeight = OverworldMap::Height * MapTileSize.y;

    if (left < 0 || top < 0 || right >= mapWidth || bottom >= mapHeight)
    {
        return std::nullopt;
    }

    int minTileX = left / MapTileSize.x;
    int minTileY = top / MapTileSize.y;
    int maxTileX = right / MapTileSize.x;
    int maxTileY = bottom / MapTileSize.y;

    for (int tileY = minTileY; tileY <= maxTileY; ++tileY)
    {
        for (int tileX = minTileX; tileX <= maxTileX; ++tileX)
        {
            if (_map.GetTileId(tileX, tileY) != EntryMarkerTileId)
            {
                continue;
            }

            int roomX = tileX / RoomTileWidth;
            int roomY = tileY / RoomTileHeight;
            RoomCoordinate room{ roomX, roomY };

            int localX = tileX % RoomTileWidth;
            int localY = tileY % RoomTileHeight;

            if (room == StartRoom &&
                localX == StartSwordCaveLocalX &&
                localY == StartSwordCaveLocalY)
            {
                return EntranceType::SwordCave;
            }

            if (room == Dungeon1EntranceRoom &&
                localX == Dungeon1EntranceLocalX &&
                localY == Dungeon1EntranceLocalY)
            {
                return EntranceType::Dungeon1;
            }
        }
    }

    return std::nullopt;
}

bool OverworldLevel::TryEnterEntrance(Vector2 destination)
{
    if (!_player) return false;

    auto entrance = ResolveEntrance(destination, *_player);
    if (!entrance) return false;

    switch (*entrance)
    {
    case EntranceType::SwordCave:
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());

        game.SetPlayerHp(_player->GetHp());
        game.SetHasSword(_player->HasSword());
        game.ChangeLevel(State::SwordCave);

        _player->ClearMoveRemainder();

        return true;
    }

    case EntranceType::Dungeon1:
    {
        if (!_player->HasSword())
        {
            return false;
        }

        Game& game = dynamic_cast<Game&>(Engine::Get());

        game.SetPlayerHp(_player->GetHp());
        game.SetHasSword(_player->HasSword());
        game.ChangeLevel(State::Dungeon1);

        _player->ClearMoveRemainder();

        return true;
    }
    }

    return false;
}
