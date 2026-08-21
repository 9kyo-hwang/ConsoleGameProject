#include "pch.h"
#include "DungeonLevel.h"

#include <Actor/Enemy.h>
#include <Actor/Aquamentus.h>
#include <Actor/Octorok.h>
#include <Actor/Player.h>
#include <Actor/Projectile.h>
#include <Actor/SwordAttack.h>

#include <Component/BoxComponent.h>

#include <Core/Input.h>
#include <Engine/Engine.h>
#include <Game/Game.h>
#include <Math/MathUtility.h>

#include <Render/Renderer.h>
#include <Render/Sprite.h>

#include <array>
#include <filesystem>
#include <set>

using namespace Craft;
using FilePath = std::filesystem::path;

namespace
{
    const Vector2 MapTileSize(10, 5);
    const Vector2 RoomScreenOffset(0, 3);

    void DrawHeartSprite(
        std::vector<SpriteCell>& cells,
        int spriteWidth,
        int tileX,
        int tileY)
    {
        const std::array<std::string, 5> art
        {
            "  ++  @@  ",
            " +@@@@@@@ ",
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

                const int destX = tileX * MapTileSize.x + x;
                const int destY = tileY * MapTileSize.y + y;

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
        const std::array<std::string, 5> art
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

                const int destX = tileX * MapTileSize.x + x;
                const int destY = tileY * MapTileSize.y + y;

                cells[destY * spriteWidth + destX] = SpriteCell(
                    glyph,
                    static_cast<WORD>(color),
                    false
                );
            }
        }
    }

    bool Overlaps(
        const Vector2& lhsPosition,
        const Vector2& lhsSize,
        const Vector2& rhsPosition,
        const Vector2& rhsSize)
    {
        const int lhsRight =
            lhsPosition.x + lhsSize.x - 1;

        const int lhsBottom =
            lhsPosition.y + lhsSize.y - 1;

        const int rhsRight =
            rhsPosition.x + rhsSize.x - 1;

        const int rhsBottom =
            rhsPosition.y + rhsSize.y - 1;

        return !(lhsRight < rhsPosition.x ||
                 rhsRight < lhsPosition.x ||
                 lhsBottom < rhsPosition.y ||
                 rhsBottom < lhsPosition.y);
    }

    bool IsInContact(const Pawn& lhs, const Pawn& rhs)
    {
        const auto lhsBox = lhs.GetComponent<BoxComponent>();
        const auto rhsBox = rhs.GetComponent<BoxComponent>();

        if (!lhsBox || !rhsBox)
        {
            return false;
        }

        const Vector2 lhsPosition =
            lhs.GetWorldPosition() + lhsBox->GetOffset();

        const Vector2 rhsPosition =
            rhs.GetWorldPosition() + rhsBox->GetOffset();

        const Vector2 lhsSize = lhsBox->GetSize();
        const Vector2 rhsSize = rhsBox->GetSize();

        const int lhsLeft = lhsPosition.x;
        const int lhsTop = lhsPosition.y;
        const int lhsRight = lhsLeft + lhsSize.x - 1;
        const int lhsBottom = lhsTop + lhsSize.y - 1;

        const int rhsLeft = rhsPosition.x;
        const int rhsTop = rhsPosition.y;
        const int rhsRight = rhsLeft + rhsSize.x - 1;
        const int rhsBottom = rhsTop + rhsSize.y - 1;

        const bool xOverlap =
            lhsLeft <= rhsRight && rhsLeft <= lhsRight;

        const bool yOverlap =
            lhsTop <= rhsBottom && rhsTop <= lhsBottom;

        const bool xAdjacent =
            lhsRight + 1 >= rhsLeft &&
            rhsRight + 1 >= lhsLeft;

        const bool yAdjacent =
            lhsBottom + 1 >= rhsTop &&
            rhsBottom + 1 >= lhsTop;

        return (xAdjacent && yOverlap) ||
               (yAdjacent && xOverlap);
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

    const Vector2 spawnPosition =
        Vector2(PlayerSpawnTileX, PlayerSpawnTileY) * MapTileSize;

    _player = SpawnActor<Player>(spawnPosition, Game::PlayerMaxHp);
    _player->SetHealth(game.GetPlayerHp());
    _needsPlayerSync = false;

    if (game.HasSword())
    {
        _player->EquipSword();
    }

    SpawnRoomEnemies();
}

void DungeonLevel::EndPlay()
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

            auto attack = SpawnActor<SwordAttack>(
                direction,
                _player,
                1
            );

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
    UpdateBossState();
    TakeContactDamageToPlayer();
}

void DungeonLevel::Draw()
{
    Renderer& renderer = Renderer::Get();

    const Vector2 roomOrigin =
        GetRoomWorldOrigin(_currentRoom);

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
        RoomTileWidth * MapTileSize.x,
        RoomTileHeight * MapTileSize.y
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
                 offsetY < MapTileSize.y;
                 ++offsetY)
            {
                for (int offsetX = 0;
                     offsetX < MapTileSize.x;
                     ++offsetX)
                {
                    const int destX =
                        localX * MapTileSize.x + offsetX;

                    const int destY =
                        localY * MapTileSize.y + offsetY;

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
        GetRoomWorldOrigin(_currentRoom);

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

        const Vector2 worldPosition =
            roomOrigin + Vector2(localX, localY) * MapTileSize;

        if (!_map.CanOccupyWorldRect(
                worldPosition,
                Vector2(10, 5),
                MapTileSize))
        {
            continue;
        }

        const Vector2 distance =
            worldPosition - _player->GetWorldPosition();

        if (std::abs(distance.x) < MapTileSize.x * 2 &&
            std::abs(distance.y) < MapTileSize.y * 2)
        {
            continue;
        }

        if (!selected.emplace(worldPosition).second)
        {
            continue;
        }

        if (auto enemy = SpawnEnemy(worldPosition))
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

    const Vector2 bossWorldPosition =
        _bossTile * MapTileSize;

    _boss = SpawnActor<Aquamentus>(bossWorldPosition);
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
        ExitTileX * MapTileSize.x;

    const int exitRight =
        (ExitTileX + ExitTileWidth) * MapTileSize.x - 1;

    const int dungeonBottom =
        DungeonMap::Height * MapTileSize.y;

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
    game.SetPlayerHp(_player->GetHp());
    game.SetHasSword(_player->HasSword());
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

    if (!_map.CanOccupyWorldRect(
            moverPosition,
            moverBox->GetSize(),
            MapTileSize))
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

            return Overlaps(
                moverPosition,
                moverBox->GetSize(),
                otherPosition,
                otherBox->GetSize()
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
    if (boxSize.x <= 0 || boxSize.y <= 0)
    {
        return false;
    }

    const Vector2 roomOrigin =
        GetRoomWorldOrigin(_currentRoom);

    const Vector2 roomSize
    {
        RoomTileWidth * MapTileSize.x,
        RoomTileHeight * MapTileSize.y
    };

    const int left = boxPosition.x;
    const int top = boxPosition.y;
    const int right = left + boxSize.x - 1;
    const int bottom = boxPosition.y + boxSize.y - 1;

    const int roomLeft = roomOrigin.x;
    const int roomTop = roomOrigin.y;
    const int roomRight = roomLeft + roomSize.x - 1;
    const int roomBottom = roomTop + roomSize.y - 1;

    return left >= roomLeft &&
           top >= roomTop &&
           right <= roomRight &&
           bottom <= roomBottom;
}

RoomCoordinate DungeonLevel::GetRoomCoordinate(
    const Vector2& worldPosition) const
{
    assert(worldPosition.x >= 0 &&
           worldPosition.y >= 0);

    const int roomWorldWidth =
        RoomTileWidth * MapTileSize.x;

    const int roomWorldHeight =
        RoomTileHeight * MapTileSize.y;

    return RoomCoordinate(
        worldPosition.x / roomWorldWidth,
        worldPosition.y / roomWorldHeight
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

Vector2 DungeonLevel::GetRoomWorldOrigin(
    RoomCoordinate room) const
{
    return Vector2
    {
        room.x * RoomTileWidth * MapTileSize.x,
        room.y * RoomTileHeight * MapTileSize.y
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

    const Vector2 roomOrigin = GetRoomWorldOrigin(room);
    const Vector2 roomSize
    {
        RoomTileWidth * MapTileSize.x,
        RoomTileHeight * MapTileSize.y
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

    return _map.CanOccupyWorldRect(
        boxPosition,
        box->GetSize(),
        MapTileSize
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
        tile * MapTileSize;

    return Overlaps(
        playerPosition,
        playerBox->GetSize(),
        itemPosition,
        MapTileSize
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
