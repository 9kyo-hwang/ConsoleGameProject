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

    if (_player)
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        if (game.HasSword())
        {
            _player->EquipSword();
        }

        return;
    }

    // 월드 좌표
    _player = SpawnActor<Player>(GetRoomWorldOrigin(_currentRoom) + Vector2(7, 2) * MapTileSize, 4);

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

    if (_player->IsDead())
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::GameOver);
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

            if (_player->IsFullHp())
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

                SpawnProjectile(_player->GetWorldPosition() + direction, swordBeam, _player);
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

    renderer.Submit("[Overworld Level]", Vector2::Zero);

    if (_player)
    {
        const std::string hp = "[HP " + std::to_string(_player->GetHp()) + "/" + std::to_string(_player->GetMaxHp()) + "]";
        renderer.Submit(hp, Vector2(2, 1));

        const std::string sword = _player->HasSword() ? "[SWORD]" : "[NO SWORD]";
        renderer.Submit(sword, Vector2(20, 1));
    }
}

void OverworldLevel::EndPlay()
{
    Level::EndPlay();

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

        enemy->Think(deltaTime, *_player);
        
        EnemyAttackRequest request;
        if (enemy->ConsumeAttackRequest(request))
        {
            SpawnProjectile(enemy->GetWorldPosition() + request.spawnOffset, request.projectile, enemy);
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
            // TODO: 추후 보스는 접촉 피해를 다르게 주고 싶으면 GetContactDamage() 같은 가상 함수 추가
            _player->TakeDamage(1, enemy, enemy);
        }
    }
}

/*
* TileId가 0x12인지 검사
* Room 좌표 확인
* Room 내부 타일 좌표 확인
* (7, 7) Room의 (4, 1) Tile이면 SwordCave
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

            // 분류되지 않은 0x12
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

        game.SetHasSword(_player->HasSword());
        game.ChangeLevel(State::SwordCave);

        _player->ClearMoveRemainder();

        return true;
    }
    }

    return false;
}
