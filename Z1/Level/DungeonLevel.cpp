#include "pch.h"
#include "DungeonLevel.h"

#include <Actor/Enemy.h>
#include <Actor/Aquamentus.h>
#include <Actor/Octorok.h>
#include <Actor/Player.h>
#include <Actor/Projectile.h>
#include <Actor/SwordAttack.h>
#include <Actor/SwordSprite.h>

#include <Component/BoxComponent.h>

#include <Core/Input.h>
#include <Engine/Engine.h>
#include <Game/Game.h>
#include <Math/MathUtility.h>
#include <Util/BoxBounds.h>
#include <World/MapGeometry.h>

#include <Render/Renderer.h>
#include <Render/Sprite.h>

#include <array>
#include <filesystem>
#include <set>

using namespace Craft;
using FilePath = std::filesystem::path;

namespace
{
    const Vector2 RoomScreenOffset(0, 3);

    void DrawHeartSprite(
        std::vector<SpriteCell>& cells,
        int spriteWidth,
        int tileX,
        int tileY)
    {
        static const std::array<std::string, 5> art
        {
            "  @@  @@  ",
            " @@@@@@@@ ",
            "  @@@@@@  ",
            "   @@@@   ",
            "    @@    "
        };

        for (int y = 0; y < static_cast<int>(art.size()); ++y)
        {
            for (int x = 0; x < static_cast<int>(art[y].size()); ++x)
            {
                const char glyph = art[y][x];
                if (glyph == ' ')
                {
                    continue;
                }

                const Color color = glyph == '+'
                    ? Color::White
                    : Color::Red;

                const int destX = tileX * TileCellSize.x + x;
                const int destY = tileY * TileCellSize.y + y;

                cells[destY * spriteWidth + destX] = SpriteCell(
                    glyph,
                    static_cast<WORD>(color),
                    false
                );
            }
        }
    }

    void DrawTriforceSprite(
        std::vector<SpriteCell>& cells,
        int spriteWidth,
        int tileX,
        int tileY)
    {
        static const std::array<std::string, 5> art
        {
            "    /\\    ",
            "   /@@\\   ",
            "  /@@@@\\  ",
            " /@@/\\@@\\ ",
            "          "
        };

        for (int y = 0; y < static_cast<int>(art.size()); ++y)
        {
            for (int x = 0; x < static_cast<int>(art[y].size()); ++x)
            {
                const char glyph = art[y][x];
                if (glyph == ' ')
                {
                    continue;
                }

                const Color color = glyph == '@'
                    ? Color::Yellow
                    : Color::DarkYellow;

                const int destX = tileX * TileCellSize.x + x;
                const int destY = tileY * TileCellSize.y + y;

                cells[destY * spriteWidth + destX] = SpriteCell(
                    glyph,
                    static_cast<WORD>(color),
                    false
                );
            }
        }
    }

    bool IsInContact(const Pawn& lhs, const Pawn& rhs)
    {
        const auto lhsBox = lhs.GetComponent<BoxComponent>();
        const auto rhsBox = rhs.GetComponent<BoxComponent>();

        if (!lhsBox || !rhsBox)
        {
            return false;
        }

        const BoxBounds lhsBounds
        {
            lhs.GetWorldPosition() + lhsBox->GetOffset(),
            lhsBox->GetSize()
        };

        const BoxBounds rhsBounds
        {
            rhs.GetWorldPosition() + rhsBox->GetOffset(),
            rhsBox->GetSize()
        };

        return lhsBounds.IsSideContact(rhsBounds);
    }
}

void DungeonLevel::OnInitialized()
{
    Level::OnInitialized();

    if (_loaded)
    {
        return;
    }

    _loaded = LoadMap();

    if (_loaded)
    {
        BuildRoomSprite();
    }
}

void DungeonLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_loaded)
    {
        return;
    }

    Game& game = dynamic_cast<Game&>(Engine::Get());

    if (!_bgmStarted)
    {
        Engine::Get().PlayBGM("Z1/12. Death Mountain Dungeon.wav");
        _bgmStarted = true;
    }

    if (!_player)
    {
        const Vector2 spawnPosition =
            Vector2(PlayerSpawnTileX, PlayerSpawnTileY) * TileCellSize;

        _player = SpawnActor<Player>(spawnPosition, Game::PlayerMaxHp);
        SpawnRoomEnemies();
    }

    if (!_playerStateLoaded)
    {
        game.LoadPlayerState(*_player);
        _playerStateLoaded = true;
    }
}

void DungeonLevel::EndPlay()
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

void DungeonLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (!_player)
    {
        return;
    }

    if (_clearPending)
    {
        _clearTimer.Tick(deltaTime);
        if (_clearTimer.TimeOver())
        {
            Game& game = dynamic_cast<Game&>(Engine::Get());
            game.ChangeLevel(State::Clear);
        }

        return;
    }

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

    const bool wasKnockback =
        UpdatePawnKnockback(*_player, deltaTime);

    if (!wasKnockback)
    {
        const Vector2 input =
            _player->GetMovementInputDirection();

        if (input != Vector2::Zero)
        {
            _player->CancelAttack();
            UpdatePlayerMovement(deltaTime, input);
        }
        else if (_player->HasSword() &&
                 Input::Get().GetKeyDown('A') &&
                 !_player->IsAttacking())
        {
            const Vector2 direction =
                _player->GetFacingDirection();
            const bool canShootSwordBeam = _player->IsFullHp();

            if (!canShootSwordBeam)
            {
                auto attack = SpawnActor<SwordAttack>(
                    direction,
                    _player,
                    1
                );

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
    UpdateBossState();
    TakeContactDamageToPlayer();
}

void DungeonLevel::Draw()
{
    Renderer& renderer = Renderer::Get();

    const Vector2 roomOrigin =
        GetRoomCellOrigin(_currentRoom);

    renderer.SetView(roomOrigin, RoomScreenOffset);

    if (_roomSprite)
    {
        renderer.SubmitWorld(
            _roomSprite,
            roomOrigin,
            0
        );
    }

    Level::Draw();

    renderer.Submit(
        "[DUNGEON 1]",
        Vector2(2, 1)
    );

    if (_player)
    {
        const std::string hp =
            "[HP " +
            std::to_string(_player->GetHp()) +
            "/" +
            std::to_string(_player->GetMaxHp()) +
            "]";

        renderer.Submit(hp, Vector2(16, 1));

        const std::string sword =
            _player->HasSword()
            ? "[SWORD]"
            : "[NO SWORD]";

        renderer.Submit(sword, Vector2(30, 1));
    }
}

bool DungeonLevel::LoadMap()
{
    const FilePath path =
        "../Content/Z1/Maps/Dungeons/Level1.txt";

    std::string error;

    if (!_map.Load(path, error))
    {
        return false;
    }

    bool hasBoss = false;
    bool hasHeart = false;
    bool hasTriforce = false;

    for (int y = 0; y < DungeonMap::Height; ++y)
    {
        for (int x = 0; x < DungeonMap::Width; ++x)
        {
            const char tile = _map.GetTile(x, y);

            if (tile == 'B')
            {
                if (hasBoss)
                {
                    return false;
                }

                _bossTile = Vector2(x, y);
                hasBoss = true;
            }
            else if (tile == 'H')
            {
                if (hasHeart)
                {
                    return false;
                }

                _heartTile = Vector2(x, y);
                hasHeart = true;
            }
            else if (tile == 'T')
            {
                if (hasTriforce)
                {
                    return false;
                }

                _triforceTile = Vector2(x, y);
                hasTriforce = true;
            }
        }
    }

    return hasBoss && hasHeart && hasTriforce;
}

bool DungeonLevel::TryChangeRoom(RoomCoordinate room)
{
    if (room == _currentRoom)
    {
        return false;
    }

    if (room.x < 0 ||
        room.x >= DungeonMap::RoomColumns ||
        room.y != 0)
    {
        return false;
    }

    DestroyRoomEnemies();
    DestroyRoomProjectiles();

    _currentRoom = room;

    BuildRoomSprite();
    SpawnRoomEnemies();

    if (_currentRoom.x == BossRoomIndex)
    {
        SpawnBoss();
    }

    return true;
}

void DungeonLevel::BuildRoomSprite()
{
    const Vector2 spriteSize
    {
        RoomTileWidth * TileCellSize.x,
        RoomTileHeight * TileCellSize.y
    };

    std::vector<SpriteCell> cells(
        spriteSize.x * spriteSize.y,
        SpriteCell()
    );

    const int originX =
        _currentRoom.x * RoomTileWidth;

    const int originY =
        _currentRoom.y * RoomTileHeight;

    for (int localY = 0;
         localY < RoomTileHeight;
         ++localY)
    {
        for (int localX = 0;
             localX < RoomTileWidth;
             ++localX)
        {
            const char tile =
                _map.GetTile(
                    originX + localX,
                    originY + localY
                );

            SpriteCell visual(
                ' ',
                (WORD)Color::Black,
                false
            );

            if (tile == '#')
            {
                visual = SpriteCell(
                    '#',
                    (WORD)Color::DarkRed,
                    false
                );
            }
            else if (tile == 'x')
            {
                visual = SpriteCell(
                    'x',
                    (WORD)Color::DarkGray,
                    false
                );
            }
            for (int offsetY = 0;
                 offsetY < TileCellSize.y;
                 ++offsetY)
            {
                for (int offsetX = 0;
                     offsetX < TileCellSize.x;
                     ++offsetX)
                {
                    const int destX =
                        localX * TileCellSize.x + offsetX;

                    const int destY =
                        localY * TileCellSize.y + offsetY;

                    const int index =
                        destY * spriteSize.x + destX;

                    cells[index] = visual;
                }
            }

            if (tile == 'H' && !_heartCollected)
            {
                DrawHeartSprite(
                    cells,
                    spriteSize.x,
                    localX,
                    localY
                );
            }
            else if (tile == 'T' && !_triforceCollected)
            {
                DrawTriforceSprite(
                    cells,
                    spriteSize.x,
                    localX,
                    localY
                );
            }
        }
    }

    _roomSprite = std::make_shared<const Sprite>(
        spriteSize,
        std::move(cells)
    );
}

void DungeonLevel::SpawnRoomEnemies()
{
    if (!_player ||
        _currentRoom.x > RandomEnemyRoomLastIndex)
    {
        return;
    }

    FMath::SetRandomSeed(
        _worldSeed ^
        static_cast<uint32_t>(_currentRoom.x) *
        73856093u
    );

    const Vector2 roomOrigin =
        GetRoomCellOrigin(_currentRoom);

    std::set<Vector2> selected;

    for (int attempt = 0;
         attempt < MaxSpawnAttempts &&
         static_cast<int>(selected.size()) < EnemyCount;
         ++attempt)
    {
        const int localX =
            FMath::RandRange(1, RoomTileWidth - 2);

        const int localY =
            FMath::RandRange(1, RoomTileHeight - 2);

        const Vector2 mapCellPosition =
            roomOrigin + Vector2(localX, localY) * TileCellSize;

        if (!_map.CanPlaceBox(
                BoxBounds{ mapCellPosition, TileCellSize }))
        {
            continue;
        }

        const Vector2 distance =
            mapCellPosition - _player->GetWorldPosition();

        if (std::abs(distance.x) < TileCellSize.x * 2 &&
            std::abs(distance.y) < TileCellSize.y * 2)
        {
            continue;
        }

        if (!selected.emplace(mapCellPosition).second)
        {
            continue;
        }

        if (auto enemy = SpawnEnemy(mapCellPosition))
        {
            _roomEnemies.emplace_back(enemy);
        }
    }
}

std::shared_ptr<Enemy> DungeonLevel::SpawnEnemy(Vector2 position)
{
    return SpawnActor<Octorok>(
        position,
        EnemyVariant::Red
    );
}

void DungeonLevel::SpawnBoss()
{
    if (_bossDefeated ||
        _boss ||
        _currentRoom.x != BossRoomIndex)
    {
        return;
    }

    const Vector2 bossMapCellPosition =
        _bossTile * TileCellSize;

    _boss = SpawnActor<Aquamentus>(bossMapCellPosition);
    Engine::Get().PlayOneShot("Z1/LOZ_Boss_Scream1.wav");

    _roomEnemies.emplace_back(_boss);
}

void DungeonLevel::DestroyRoomEnemies()
{
    for (const auto& enemy : _roomEnemies)
    {
        if (enemy)
        {
            enemy->Destroy();
        }
    }

    _roomEnemies.clear();

    if (!_bossDefeated)
    {
        _boss.reset();
    }
}

void DungeonLevel::DestroyRoomProjectiles()
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

bool DungeonLevel::TryExitDungeon(
    const Vector2& destination,
    const Vector2& moveDelta)
{
    if (!_player ||
        moveDelta != Vector2::Up * -1)
    {
        return false;
    }

    const auto box = _player->GetComponent<BoxComponent>();
    if (!box)
    {
        return false;
    }

    const Vector2 boxPosition =
        destination + box->GetOffset();

    const Vector2 boxSize =
        box->GetSize();

    const int left = boxPosition.x;
    const int right = left + boxSize.x - 1;
    const int bottom = boxPosition.y + boxSize.y - 1;

    constexpr int ExitTileX = 7;
    constexpr int ExitTileWidth = 2;

    const int exitLeft =
        ExitTileX * TileCellSize.x;

    const int exitRight =
        (ExitTileX + ExitTileWidth) * TileCellSize.x - 1;

    const int dungeonBottom =
        DungeonMap::Height * TileCellSize.y;

    const bool overlapsExitWidth =
        left <= exitRight && right >= exitLeft;

    const bool reachesDungeonBoundary =
        bottom >= dungeonBottom;

    if (!overlapsExitWidth ||
        !reachesDungeonBoundary)
    {
        return false;
    }

    Game& game = dynamic_cast<Game&>(Engine::Get());
    game.ChangeLevel(State::Overworld);

    return true;
}

void DungeonLevel::UpdatePlayerMovement(
    float deltaTime,
    const Vector2& delta)
{
    const int moveSteps =
        _player->ConsumeMoveSteps(deltaTime);

    for (int i = 0; i < moveSteps; ++i)
    {
        const Vector2 candidate =
            _player->GetWorldPosition() + delta;

        if (TryExitDungeon(candidate, delta))
        {
            _player->ClearMoveRemainder();
            return;
        }

        if (!CanMoveTo(candidate, *_player))
        {
            _player->ClearMoveRemainder();
            break;
        }

        const RoomCoordinate nextRoom =
            GetRoomCoordinateAtLeadingEdge(
                candidate,
                *_player,
                delta
            );

        _player->MoveBy(delta);

        if (nextRoom != _currentRoom)
        {
            SnapPlayerIntoRoom(nextRoom, delta);
            TryChangeRoom(nextRoom);
        }

        TryCollectItems();
    }
}

void DungeonLevel::UpdateEnemyMovement(float deltaTime)
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

        const Vector2 delta =
            enemy->GetDesiredMove();

        if (UpdatePawnKnockback(*enemy, deltaTime))
        {
            continue;
        }

        const int moveSteps =
            enemy->ConsumeMoveSteps(deltaTime);

        for (int i = 0; i < moveSteps; ++i)
        {
            const Vector2 candidate =
                enemy->GetWorldPosition() + delta;

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

bool DungeonLevel::UpdatePawnKnockback(
    Pawn& pawn,
    float deltaTime)
{
    if (!pawn.IsKnockback())
    {
        return false;
    }

    const Vector2 direction =
        pawn.GetKnockbackDirection();

    const int steps =
        pawn.ConsumeKnockbackSteps(deltaTime);

    for (int i = 0; i < steps; ++i)
    {
        const Vector2 destination =
            pawn.GetWorldPosition() + direction;

        if (!CanMoveTo(destination, pawn))
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

bool DungeonLevel::CanMoveTo(
    const Vector2& destination,
    const Pawn& mover)
{
    const auto moverBox =
        mover.GetComponent<BoxComponent>();

    if (!moverBox)
    {
        return false;
    }

    const Vector2 moverPosition =
        destination + moverBox->GetOffset();

    if (mover.IsA<Enemy>() &&
        !IsInsideCurrentRoom(
            moverPosition,
            moverBox->GetSize()))
    {
        return false;
    }

    if (!_map.CanPlaceBox(
            BoxBounds{ moverPosition, moverBox->GetSize() }))
    {
        return false;
    }

    auto IsOverlappingPawn =
        [&mover, &moverPosition, &moverBox]
        (const std::shared_ptr<Pawn>& other)
        {
            if (!other ||
                !other->IsActive() ||
                other.get() == &mover)
            {
                return false;
            }

            const auto otherBox =
                other->GetComponent<BoxComponent>();

            if (!otherBox)
            {
                return false;
            }

            const Vector2 otherPosition =
                other->GetWorldPosition() +
                otherBox->GetOffset();

            return BoxBounds{ moverPosition, moverBox->GetSize() }.Overlaps(
                BoxBounds{ otherPosition, otherBox->GetSize() }
            );
        };

    if (IsOverlappingPawn(_player))
    {
        return false;
    }

    // Enemy끼리는 통과 가능
    if (mover.IsA<Enemy>())
    {
        return true;
    }

    for (const auto& enemy : _roomEnemies)
    {
        if (IsOverlappingPawn(enemy))
        {
            return false;
        }
    }

    return true;
}

bool DungeonLevel::IsInsideCurrentRoom(
    Vector2 boxPosition,
    Vector2 boxSize) const
{
    const Vector2 roomOrigin =
        GetRoomCellOrigin(_currentRoom);

    const Vector2 roomSize
    {
        RoomTileWidth * TileCellSize.x,
        RoomTileHeight * TileCellSize.y
    };

    return BoxBounds{ boxPosition, boxSize }.IsInside(
        BoxBounds{ roomOrigin, roomSize }
    );
}

RoomCoordinate DungeonLevel::GetRoomCoordinate(
    const Vector2& mapCellPosition) const
{
    assert(mapCellPosition.x >= 0 &&
           mapCellPosition.y >= 0);

    const int roomCellWidth =
        RoomTileWidth * TileCellSize.x;

    const int roomCellHeight =
        RoomTileHeight * TileCellSize.y;

    return RoomCoordinate(
        mapCellPosition.x / roomCellWidth,
        mapCellPosition.y / roomCellHeight
    );
}

RoomCoordinate DungeonLevel::GetRoomCoordinateAtLeadingEdge(
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

Vector2 DungeonLevel::GetRoomCellOrigin(
    RoomCoordinate room) const
{
    return Vector2
    {
        room.x * RoomTileWidth * TileCellSize.x,
        room.y * RoomTileHeight * TileCellSize.y
    };
}

void DungeonLevel::SnapPlayerIntoRoom(
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

bool DungeonLevel::CanProjectileOccupy(
    Vector2 destination,
    const Projectile& projectile)
{
    const auto box =
        projectile.GetComponent<BoxComponent>();

    if (!box)
    {
        return false;
    }

    const Vector2 boxPosition =
        destination + box->GetOffset();

    if (!IsInsideCurrentRoom(
            boxPosition,
            box->GetSize()))
    {
        return false;
    }

    return _map.CanPlaceBox(
        BoxBounds{ boxPosition, box->GetSize() }
    );
}

std::shared_ptr<Projectile> DungeonLevel::SpawnProjectile(
    Vector2 position,
    const ProjectileSpec& spec,
    const std::shared_ptr<Pawn>& instigator)
{
    auto projectile = SpawnActor<Projectile>(
        position,
        spec,
        instigator
    );

    if (projectile)
    {
        _roomProjectiles.emplace_back(projectile);
    }

    return projectile;
}

void DungeonLevel::TakeContactDamageToPlayer()
{
    if (!_player || _player->IsDead())
    {
        return;
    }

    for (const auto& enemy : _roomEnemies)
    {
        if (!enemy ||
            !enemy->IsActive() ||
            enemy->IsDead())
        {
            continue;
        }

        if (IsInContact(*_player, *enemy))
        {
            _player->TakeDamage(
                enemy->GetContactDamage(),
                enemy,
                enemy
            );
        }
    }
}

void DungeonLevel::UpdateBossState()
{
    if (!_boss ||
        !_boss->IsDead() ||
        _bossDefeated)
    {
        return;
    }

    _bossDefeated = true;
    BuildRoomSprite();
}

bool DungeonLevel::IsPlayerOverlappingTile(
    const Vector2& tile) const
{
    if (!_player)
    {
        return false;
    }

    const auto playerBox =
        _player->GetComponent<BoxComponent>();

    if (!playerBox)
    {
        return false;
    }

    const Vector2 playerPosition =
        _player->GetWorldPosition() + playerBox->GetOffset();

    const Vector2 itemPosition =
        tile * TileCellSize;

    return BoxBounds{ playerPosition, playerBox->GetSize() }.Overlaps(
        BoxBounds{ itemPosition, TileCellSize }
    );
}

void DungeonLevel::TryCollectItems()
{
    if (!_bossDefeated)
    {
        return;
    }

    if (!_heartCollected &&
        IsPlayerOverlappingTile(_heartTile))
    {
        _player->RestoreFullHealth();
        Engine::Get().PlayOneShot("Z1/07. Collect Item.wav");
        _heartCollected = true;
        BuildRoomSprite();
    }

    if (!_triforceCollected &&
        IsPlayerOverlappingTile(_triforceTile))
    {
        _triforceCollected = true;
        _clearPending = true;
        _clearTimer.Reset();

        Engine::Get().StopBGM();
        Engine::Get().PlayOneShot("Z1/14. Zelda Is Rescued.wav");
    }
}
