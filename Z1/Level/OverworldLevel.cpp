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
#include <Actor/SwordSprite.h>
#include <Actor/Enemy.h>
#include <Math/MathUtility.h>
#include <set>
#include <Actor/Projectile.h>
#include <Actor/Moblin.h>
#include <Actor/Tektite.h>
#include <Game/Game.h>
#include <World/MapGeometry.h>

using namespace Craft;
using FilePath = std::filesystem::path;

/*
* Room의 타일 좌표: 16 x 11
* Actor 위치 및 Box 크기: Map Cell 단위
* Room 하나의 실제 월드 크기: (16 x 11) x WorldRenderScale = (80 x 33)
* RoomOrigin: Map Cell 단위
* 렌더러에 전달하는 Actor 위치: 콘솔 Cell과 1:1인 월드 좌표
* 
* 예: 타일 (7, 4)
* - LTRB: 35, 12, 39, 14
*/

namespace
{
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

    bool IsInContact(const Pawn& lhs, const Pawn& rhs)
    {
        const std::shared_ptr<BoxComponent>& lhsBox = lhs.GetComponent<BoxComponent>();
        const std::shared_ptr<BoxComponent>& rhsBox = rhs.GetComponent<BoxComponent>();

        if (!lhsBox || !rhsBox) return false;

        const Box2D lhsBounds
        {
            lhs.GetWorldPosition() + lhsBox->GetOffset(),
            lhsBox->GetSize()
        };

        const Box2D rhsBounds
        {
            rhs.GetWorldPosition() + rhsBox->GetOffset(),
            rhsBox->GetSize()
        };

        return lhsBounds.IsSideContact(rhsBounds);
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

    if (!_player)
    {
        // 월드 좌표
        _player = SpawnActor<Player>(
            GetRoomCellOrigin(_currentRoom) + Vector2(7, 2) * TileCellSize,
            Game::PlayerMaxHp
        );
    }

    if (!_playerStateLoaded)
    {
        game.LoadPlayerState(*_player);
        _playerStateLoaded = true;
    }
}

void OverworldLevel::Tick(float deltaTime)
{
    Game& game = dynamic_cast<Game&>(Engine::Get());
    game.PumpNetwork(); // OverworldLevel에서, 메인 스레드가 네트워크 큐를 소비하도록
    // 이후 Player/MyPlayer 위치 설정할 때 Actor::Tick 보다 먼저 서버 확정 상태를 반영할 수 있음

    Level::Tick(deltaTime);

    if (!_player) return;

    // Level::Tick()에서 Player::Tick()이 호출되며 입력 벡터가 기록됨
    // 따라서 해당 입력 값이 기록된 직후 서버에 입력 정보를 전송
    SendNetworkInput(game);

    // 싱글플레이 로직

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
            const bool canShootSwordBeam = _player->IsFullHp();

            if (!canShootSwordBeam)
            {
                auto attack = SpawnActor<SwordAttack>(direction, _player, 1);
                attack->AttachTo(_player, false);
                _player->SetActiveAttack(attack);
            }

            Engine::Get().PlayOneShot(
                canShootSwordBeam
                ? "Z1/LOZ_Sword_Combined.wav"
                : "Z1/LOZ_Sword_Slash.wav"
            );

            if (canShootSwordBeam)
            {
                const auto swordSprite = CreateSwordSprite(direction);
                ProjectileSpec swordBeam
                {
                    .type = ProjectileType::SwordBeam,
                    .faction = ProjectileFaction::Player,
                    .damage = 1,
                    .speed = 40.f,
                    .lifetime = 1.5f,
                    .direction = direction,
                    .boxSize = swordSprite->GetSize(),
                    .sprite = swordSprite,
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
    Game& game = dynamic_cast<Game&>(Engine::Get());

    const Vector2 roomCellOrigin = GetRoomCellOrigin(_currentRoom);
    renderer.SetView(roomCellOrigin, RoomScreenOffset);

    if (_roomSprite)
    {
        renderer.SubmitWorld(
            _roomSprite, 
            roomCellOrigin,
            0
        );
    }

    Level::Draw();

    renderer.Submit(Sprite::Create("[Overworld]"), Vector2(2, 1));

    if (_player)
    {
        const std::string hp = "[HP " + std::to_string(_player->GetHp()) + "/" + std::to_string(_player->GetMaxHp()) + "]";
        renderer.Submit(Sprite::Create(hp), Vector2(16, 1));

        const std::string sword = _player->HasSword() ? "[SWORD]" : "[NO SWORD]";
        renderer.Submit(Sprite::Create(sword), Vector2(30, 1));
    }

    const Vector2 NetHUDPos(46, 1);
    std::string netText;
    
    if (!game.IsServerConnected())
    {
        netText = "[OFFLINE]";
    }
    else
    {
        const auto localPlayerId = game.GetLocalPlayerId();
        const auto& snapshot = game.GetLatestSnapshot();

        if (!localPlayerId.has_value())
        {
            netText = "[Connecting...]";
        }
        else if (!snapshot.has_value())
        {
            // PlayerId는 받았는데 snapshot 받기 전
            netText = "[ONLINE] Player " + std::to_string(*localPlayerId);
        }
        else
        {
            netText = "[ONLINE] Player " + std::to_string(*localPlayerId)
                + " Tick " + std::to_string(snapshot->serverTick)
                + " Num " + std::to_string(snapshot->players.size());
        }
    }

    renderer.Submit(Sprite::Create(netText), NetHUDPos);
}

void OverworldLevel::EndPlay()
{
    Level::EndPlay();

    if (_player)
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.SavePlayerState(*_player);
    }

    _playerStateLoaded = false;

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

    return _map.CanPlaceBox(Box2D{ boxPos, box->GetSize() });
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
    case EnemyKind::Octorok: return SpawnActor<Octorok>(spawn.mapCellPosition, spawn.variant);
    case EnemyKind::Moblin: return SpawnActor<Moblin>(spawn.mapCellPosition, spawn.variant);
    case EnemyKind::Tektite: return SpawnActor<Tektite>(spawn.mapCellPosition, spawn.variant);
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
    /*
    * 현재 Room 좌표에 해당하는 부분 Sprite를
    * Map에 질의해 가져오자
    */

    // 논리 타일 위치: (16, 11) x (7, 7)
    const Vector2 roomOrigin(_currentRoom.x * RoomTileWidth, _currentRoom.y * RoomTileHeight);
    _roomSprite = _map.BuildRoomSprite(roomOrigin, Vector2(RoomTileWidth, RoomTileHeight));
}

/// <summary>
/// 현재 위치(전체 맵에 Cell 좌표 기준)가 어디 Room에 속하는지
/// </summary>
/// <param name="mapCellPosition"></param>
/// <returns></returns>
RoomCoordinate OverworldLevel::GetRoomCoordinate(const Vector2& mapCellPosition) const
{
    assert(mapCellPosition.x >= 0 && mapCellPosition.y >= 0);

    // Room의 셀 단위 가로/세로 길이
    const int roomCellWidth =  RoomTileWidth * TileCellSize.x;
    const int roomCellHeight = RoomTileHeight * TileCellSize.y;

    return RoomCoordinate(mapCellPosition.x / roomCellWidth, mapCellPosition.y / roomCellHeight);
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
Vector2 OverworldLevel::GetRoomCellOrigin(RoomCoordinate room) const
{
    return Vector2
    {
        room.x * RoomTileWidth * TileCellSize.x,
        room.y * RoomTileHeight * TileCellSize.y
    };
}

void OverworldLevel::SnapPlayerIntoRoom(
    RoomCoordinate room,
    const Vector2& direction)
{
    if (!_player)
    {
        return;
    }

    const auto box = _player->GetComponent<BoxComponent>();
    if (!box)
    {
        return;
    }

    const Vector2 roomOrigin = GetRoomCellOrigin(room);
    const Vector2 roomSize
    {
        RoomTileWidth * TileCellSize.x,
        RoomTileHeight * TileCellSize.y
    };

    Vector2 position = _player->GetWorldPosition();
    const Vector2 offset = box->GetOffset();
    const Vector2 size = box->GetSize();

    if (direction.x > 0)
    {
        position.x = roomOrigin.x - offset.x;
    }
    else if (direction.x < 0)
    {
        position.x = roomOrigin.x + roomSize.x - size.x - offset.x;
    }
    else if (direction.y > 0)
    {
        position.y = roomOrigin.y - offset.y;
    }
    else if (direction.y < 0)
    {
        position.y = roomOrigin.y + roomSize.y - size.y - offset.y;
    }

    _player->SetPosition(position);
}

bool OverworldLevel::CanMoveTo(
    const Craft::Vector2& destination,
    const Pawn& mover,
    bool allowContactEscape)
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

    if (!mover.IsA<Tektite>() &&
        !_map.CanPlaceBox(
            Box2D{ moverPosition, moverBox->GetSize() }))
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

            return Box2D{ moverPosition, moverBox->GetSize() }.Overlaps(
                Box2D{ otherPosition, otherBox->GetSize() }
            );
        };

    if (IsOverlapping(_player) && !mover.IsA<Tektite>())
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
            const auto enemyBox =
                enemy->GetComponent<BoxComponent>();

            const bool isEscapingContact =
                mover.IsA<Player>() &&
                allowContactEscape &&
                enemyBox &&
                Box2D{
                    mover.GetWorldPosition() + moverBox->GetOffset(),
                    moverBox->GetSize()
                }.Overlaps(
                    Box2D{
                        enemy->GetWorldPosition() + enemyBox->GetOffset(),
                        enemyBox->GetSize()
                    }
                );

            if (!isEscapingContact)
            {
                return false;
            }
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
        if (!CanMoveTo(destination, pawn, pawn.IsA<Player>()))
        {
            pawn.StopKnockback();
            break;
        }

        RoomCoordinate nextRoom = _currentRoom;
        if (pawn.IsA<Player>())
        {
            nextRoom = GetRoomCoordinateAtLeadingEdge(
                destination,
                pawn,
                direction
            );

            if (!IsAccessibleRoom(nextRoom))
            {
                pawn.StopKnockback();
                break;
            }
        }

        pawn.MoveBy(direction);

        if (pawn.IsA<Player>())
        {
            if (nextRoom != _currentRoom)
            {
                SnapPlayerIntoRoom(nextRoom, direction);
            }

            TryChangeRoom(nextRoom);
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
        _worldSeed,
        GetRoomCellOrigin(_currentRoom)
    );

    for (const auto& spawn : spawnPlan)
    {
        if (auto enemy = SpawnEnemy(spawn))
        {
            _roomEnemies.emplace_back(enemy);
        }
        //_roomEnemies.push_back(SpawnActor<Enemy>(spawn.mapCellPosition, spawn.maxHp));
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

        _player->MoveBy(delta);
        if (nextRoom != _currentRoom)
        {
            SnapPlayerIntoRoom(nextRoom, delta);
        }

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
    const Vector2 origin = GetRoomCellOrigin(_currentRoom);
    const Vector2 roomSize{ RoomTileWidth * TileCellSize.x, RoomTileHeight * TileCellSize.y };

    return Box2D{ boxPosition, boxSize }.IsInside(
        Box2D{ origin, roomSize }
    );
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

    int mapWidth = OverworldMap::Width * TileCellSize.x;
    int mapHeight = OverworldMap::Height * TileCellSize.y;

    if (left < 0 || top < 0 || right >= mapWidth || bottom >= mapHeight)
    {
        return std::nullopt;
    }

    int minTileX = left / TileCellSize.x;
    int minTileY = top / TileCellSize.y;
    int maxTileX = right / TileCellSize.x;
    int maxTileY = bottom / TileCellSize.y;

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

        game.ChangeLevel(State::Dungeon1);

        _player->ClearMoveRemainder();

        return true;
    }
    }

    return false;
}

void OverworldLevel::SendNetworkInput(Game& game)
{
    using namespace Z1::Protocol;

    MoveDirection dir = MoveDirection::None;
    std::uint8_t actionFlags = 0;

    // 불필요한 송신을 줄이기 위한 최소한의 필터
    if (!_player->IsDead())
    {
        const Vector2 moveInputDir = _player->GetMovementInputDirection();
        if (moveInputDir == Vector2::Up) dir = MoveDirection::Up;
        else if (moveInputDir == Vector2::Up * -1) dir = MoveDirection::Down;
        else if (moveInputDir == Vector2::Right) dir = MoveDirection::Right;
        else if (moveInputDir == Vector2::Right * -1) dir = MoveDirection::Left;

        // 공격 유무를 따로 검사: 공격 버튼을 눌렀다는 의도만 전송, 아래 판단은 서버 책임
        // 검 보유 여부, 사망 여부, 공격 중인지, 쿨타임 끝났는지, ...
        if (Input::Get().GetKeyDown('A'))
        {
            actionFlags |= InputActionAttack;
        }
    }

    bool rotated = dir != _lastSentDir;
    bool hasAction = actionFlags != 0;

    if (!rotated && !hasAction) return;

    // 현재는 이동 중에 A를 누르면 방향과 액션 플래그가 함께 전송됨 -> 서버에서 시뮬 후 이동+공격 동시 허용 결정
    if (game.SendNetworkInput(dir, actionFlags))
    {
        _lastSentDir = dir;
    }
}
