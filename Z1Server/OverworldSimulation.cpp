#include "pch.h"
#include "OverworldSimulation.h"
#include <algorithm>
#include <fstream>
#include <array>
#include <random>
#include <Actor/Enemy.h>

using namespace Z1::Protocol;

namespace
{
    constexpr std::int32_t PlayerMoveCellsPerTick = 1;
    
    // 클라쪽 크기를 일단 가져왔는데...
    constexpr std::int32_t PlayerBoxWidth = 8;
    constexpr std::int32_t PlayerBoxHeight = 5;
    constexpr std::int32_t EnemiesPerRoom = 6;
    constexpr std::int32_t MaxSpawnAttempts = 30;

    std::uint32_t MakeRoomSeed(ServerRoomCoordinate room, std::uint32_t seed)
    {
        return seed ^ (std::uint32_t)room.x * 73856093u ^ (std::uint32_t)room.y * 19349663u;
    }
    
    // 타일 하나를 구성하는 셀
    constexpr std::int32_t TileCellWidth = 10;
    constexpr std::int32_t TileCellHeight = 5;
    
    // Room 하나를 구성하는 논리적 칸 개수
    constexpr std::int32_t RoomWidth = 16;
    constexpr std::int32_t RoomHeight = 11;

    // Room 하나를 구성하는 셀
    constexpr std::int32_t RoomCellWidth = RoomWidth * TileCellWidth;
    constexpr std::int32_t RoomCellHeight = RoomHeight * TileCellHeight;
    
    // Map을 구성하는 Room 개수
    constexpr std::int32_t OverworldRoomColumns = 16;
    constexpr std::int32_t OverworldRoomRows = 8;

    // Map을 구성하는 논리적 칸 개수
    constexpr std::int32_t MapWidth = OverworldRoomColumns * RoomWidth;
    constexpr std::int32_t MapHeight = OverworldRoomRows * RoomHeight;

    // Map을 구성하는 셀 칸 개수
    constexpr std::int32_t MapCellWidth = MapWidth * TileCellWidth;
    constexpr std::int32_t MapCellHeight = MapHeight * TileCellHeight;

    const ServerRoomCoordinate StartRoom{ 7, 7 };

    SnapshotPlayerState ToSnapshot(const OverworldSimulation::ServerPlayerState& state)
    {
        SnapshotPlayerState snapshot;
        snapshot.playerId = state.playerId;
        snapshot.x = state.x;
        snapshot.y = state.y;
        snapshot.hp = state.hp;
        snapshot.facing = state.facing;

        if (state.dead)
        {
            snapshot.flags |= PlayerStateDead;
        }

        return snapshot;
    }

    TileCoordinate ToLocalTile(ServerRoomCoordinate room, Vector2Int position)
    {
        return
        {
            // 논리 타일 좌표로 바꾼 걸 Room 내 상대 좌표로 컨버팅
            position.x / TileCellWidth - room.x * RoomWidth,
            position.y / TileCellHeight - room.y * RoomHeight
        };
    }

    Vector2Int ToWorldCellPosition(ServerRoomCoordinate room, TileCoordinate tile)
    {
        return
        {
            // room 내 상대 논리 타일 좌표를 전체 맵의 셀 기준 좌표로 컨버팅
            (room.x * RoomWidth + tile.x) * TileCellWidth,
            (room.y * RoomHeight + tile.y) * TileCellHeight
        };
    }

    // 투사체 발사에 사용되는, 가로/세로 중 더 큰 축
    MoveDirection GetDirectionToward(Vector2Int from, Vector2Int to)
    {
        const Vector2Int delta = to - from;
        if (std::abs(delta.x) >= std::abs(delta.y))
        {
            if (delta.x > 0) return MoveDirection::Right;
            if (delta.x < 0) return MoveDirection::Left;
        }

        if (delta.y > 0) return MoveDirection::Down;
        if (delta.y < 0) return MoveDirection::Up;

        return MoveDirection::None;
    }
}

bool OverworldSimulation::LoadBlockingMap(const FilePath& path, std::string& errorMessage)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        errorMessage = "BlockingMap file could not be opened.";
        return false;
    }

    std::vector<std::uint8_t> blocked(MapWidth * MapHeight);
    std::string line;

    for (int row = 0; row < MapHeight; ++row)
    {
        if (!std::getline(file, line))
        {
            errorMessage = "BlockingMap has fewer than 88 rows.";
            return false;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.size() != MapWidth)
        {
            errorMessage = "BlockingMap row must contains 256 characters.";
            return false;
        }

        for (int col = 0; col < MapWidth; ++col)
        {
            if (line[col] == '.')
            {
                blocked[row * MapWidth + col] = 0;
            }
            else if (line[col] == 'X')
            {
                blocked[row * MapWidth + col] = 1;
            }
            else
            {
                errorMessage = "BlockingMap map contains an invalid character.";
                return false;
            }
        }
    }

    if (std::getline(file, line))
    {
        errorMessage = "BlockingMap has more than 88 rows.";
        return false;
    }

    _blockedTiles = std::move(blocked);
    _hasBlockingMap = true;

    return true;
}

bool OverworldSimulation::SpawnEnemies(std::uint32_t seed)
{
    using RNG = std::mt19937;
    using DistInt = std::uniform_int_distribution<std::int32_t>;

    if (!_hasBlockingMap) return false;

    _enemies.clear();
    _enemyId = 1;

    for (std::int32_t roomY = 0; roomY < OverworldRoomRows; ++roomY)
    {
        for (std::int32_t roomX = 0; roomX < OverworldRoomColumns; ++roomX)
        {
            const ServerRoomCoordinate room{ roomX, roomY };
            if (room == StartRoom)
            {
                continue;
            }

            // TODO: InitialEnemySpawn 기반 Build?
            // 좌표 검증은 범용 CanPlaceBox() 추가해서 진행하기?

            RNG rng(MakeRoomSeed(room, seed));
            DistInt localTileX(1, RoomWidth - 2);
            DistInt localTileY(1, RoomHeight - 2);
            DistInt enemyKind(0, 2);

            std::int32_t spawned = 0;
            for (std::int32_t attempt = 0; attempt < MaxSpawnAttempts && spawned < EnemiesPerRoom; ++attempt)
            {
                std::int32_t x = room.x * RoomCellWidth + localTileX(rng) * TileCellWidth;
                std::int32_t y = room.y * RoomCellHeight + localTileY(rng) * TileCellHeight;
                if (!CanPlaceBox(x, y, Enemy::BoxWidth, Enemy::BoxHeight))
                {
                    continue;
                }

                EnemyKind kind = (EnemyKind)enemyKind(rng);
                int32_t id = _enemyId++;
                Vector2Int position(x, y);

                Enemy enemy(id, kind, room, position);
                _enemies.emplace(id, std::move(enemy));
                ++spawned;
            }
        }
    }

    return true;
}

bool OverworldSimulation::SpawnProjectile(Enemy& enemy, const ServerPlayerState& target)
{
    const Vector2Int targetPos = Vector2Int(target.x, target.y);
    const MoveDirection direction = GetDirectionToward(enemy.GetPosition(), targetPos);
    if (direction == MoveDirection::None) return false;

    const Vector2Int spawnPos = enemy.GetProjectileSpawnPosition();
    if (GetRoomAt(spawnPos.x, spawnPos.y) != enemy.GetHomeRoom()) return false;
    if (!CanPlaceProjectile(spawnPos.x, spawnPos.y)) return false;

    std::uint32_t id = _projectileId;
    ++_projectileId;

    Projectile projectile(id, ProjectileKind::Spear, enemy.GetId(), enemy.GetHomeRoom(), spawnPos, direction, 1, 40);

    return _projectiles.emplace(id, std::move(projectile)).second;
}

/*
* Tile: (10, 5) / Room: (16, 11)
* StartRoom: (7, 7) / LocalSpawn: (7, 2)
* 따라서 전체 Map 좌표 기준으로 (16, 11) x (10, 5) x (7, 7) + (7, 2) * (10, 5) = (1190, 395)
*/
bool OverworldSimulation::AddPlayer(std::uint32_t playerId)
{
    // 일단 플레이어끼리는 충돌 안시킬 것
    ServerPlayerState player;
    player.playerId = playerId;
    player.x = 1190;
    player.y = 395;

    return _players.emplace(playerId, std::move(player)).second;
}

void OverworldSimulation::RemovePlayer(std::uint32_t playerId)
{
    // 삭제할 id가 없으면 0 반환
    _players.erase(playerId);
}

/// <summary>
/// 클라이언트는 키 상태가 바뀔 때 입력을 보내고, 서버는 Tick마다 마지막 상태를 적용함
/// 예를 들어
/// - Up 입력을 수신하면 -> 최신 입력 = Up
/// - 다음 서버 Tick(여러 번): 서버가 계속 Up 방향으로 이동
/// - 클라이언트가 키 입력을 놓음 -> 서버는 None 입력 수신
/// - 다음 서버 Tick: 이동 정지
/// </summary>
bool OverworldSimulation::SetInput(std::uint32_t playerId, const Z1::Protocol::InputCommand& input)
{
    auto found = _players.find(playerId);
    if (found == _players.end())
    {
        return false;
    }

    ServerPlayerState& player = found->second;
    if (player.lastInputSequence.has_value() && input.sequence <= *player.lastInputSequence)
    {
        // TCP는 순서를 보장해서, 더 오래된 입력 시퀀스가 들어오는 케이스는 보통 없음
        // 중복되거나 오래된 입력은 무시
        std::cout << "C2S_Input ignored: player=" << playerId << ", sequence=" << input.sequence << "\n";
        return true;
    }

    player.lastInputSequence = input.sequence;
    player.latestInput = input;

    std::cout << "[C2S_Input] player=" << playerId
        << ", sequence=" << input.sequence
        << ", direction=" << (int)input.moveDirection
        << ", actions=" << (int)input.actionFlags
        << ", position=" << player.x << ", " << player.y << ")\n";
    return true;
}

// 각 Session이 속한 Room의 Snapshot을 반환
WorldSnapshot OverworldSimulation::BuildSnapshot(std::uint32_t id)
{
    WorldSnapshot snapshot;
    snapshot.serverTick = _tick;

    auto found = _players.find(id);
    if (found == _players.end())
    {
        return snapshot;
    }

    const auto& myPlayer = found->second;
    const auto room = GetRoomAt(myPlayer.x, myPlayer.y);
    if (!room) return snapshot;

    auto& players = snapshot.players;
    auto& enemies = snapshot.enemies;
    auto& projectiles = snapshot.projectiles;

    for (const auto& [id, player] : _players)
    {
        if (GetRoomAt(player.x, player.y) == room)
        {
            players.push_back(ToSnapshot(player));
        }
    }

    for (const auto& [id, enemy] : _enemies)
    {
        if (!enemy.IsDead() && enemy.GetHomeRoom() == *room)
        {
            enemies.push_back(enemy.BuildSnapshot());
        }
    }

    for (const auto& [id, projectile] : _projectiles)
    {
        if (!projectile.IsExpired() && projectile.GetHomeRoom() == *room)
        {
            projectiles.push_back(projectile.BuildSnapshot());
        }
    }

    std::sort(players.begin(), players.end(), 
        [](const SnapshotPlayerState& lhs, const SnapshotPlayerState& rhs)
        {
            return lhs.playerId < rhs.playerId;
        });

    std::sort(enemies.begin(), enemies.end(), 
        [](const SnapshotEnemyState& lhs, const SnapshotEnemyState& rhs)
        {
            return lhs.id < rhs.id;
        });

    std::sort(projectiles.begin(), projectiles.end(), 
        [](const SnapshotProjectileState& lhs, const SnapshotProjectileState& rhs)
        {
            return lhs.id < rhs.id;
        });

    return snapshot;
}

void OverworldSimulation::Tick()
{
    ++_tick;

    // 1. 플레이어 위치 갱신하고
    for (auto& [id, player] : _players)
    {
        if (player.dead)
        {
            continue;
        }

        const MoveDirection direction = player.latestInput.moveDirection;
        if (direction == MoveDirection::None)
        {
            continue;
        }

        // 입력이 있으면 막히더라도 방향은 바뀜
        player.facing = direction;

        const Vector2Int delta = GetMoveDelta(direction);
        for (std::int32_t step = 0; step < PlayerMoveCellsPerTick; ++step)
        {
            const std::int32_t candidateX = player.x + delta.x;
            const std::int32_t candidateY = player.y + delta.y;

            if (CanPlacePlayer(candidateX, candidateY))
            {
                player.x = candidateX;
                player.y = candidateY;
            }
        }
    }

    // 2. player가 위치한 Room 받아오고
    std::array<bool, OverworldRoomColumns * OverworldRoomRows> activeRooms{};
    for (const auto& [id, player] : _players)
    {
        if (player.dead) continue;

        if (const auto room = GetRoomAt(player.x, player.y))
        {
            ServerRoomCoordinate r = *room;
            activeRooms[r.y * OverworldRoomColumns + r.x] = true;
        }
    }

    // 3. 적 목록에서 activeRoom에 속한 것들 활성화
    for (auto& [id, enemy] : _enemies)
    {
        const ServerRoomCoordinate homeRoom = enemy.GetHomeRoom();
        if (enemy.IsDead() || !activeRooms[homeRoom.y * OverworldRoomColumns + homeRoom.x])
        {
            continue;
        }

        TickEnemy(enemy);
    }

    // 4. 투사체 목록에서 activeRoom에 속한 것들 Tick 및 Expired된 것 제거
    for (auto it = _projectiles.begin(); it != _projectiles.end();)
    {
        Projectile& projectile = it->second;
        const ServerRoomCoordinate home = projectile.GetHomeRoom();
        const bool isActiveRoom = activeRooms[home.y * OverworldRoomColumns + home.x];

        if (!isActiveRoom || !TickProjectile(projectile))
        {
            it = _projectiles.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

bool OverworldSimulation::CanPlaceBox(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) const
{
    if (!_hasBlockingMap) return false;
    if (!(x >= 0 && y >= 0 && x + width <= MapCellWidth && y + height <= MapCellHeight))
    {
        return false;
    }

    // Box 크기 계산
    const std::int32_t l = x / TileCellWidth;
    const std::int32_t t = y / TileCellHeight;
    const std::int32_t r = (x + width - 1) / TileCellWidth;
    const std::int32_t b = (y + height - 1) / TileCellHeight;

    for (int tileY = t; tileY <= b; ++tileY)
    {
        for (int tileX = l; tileX <= r; ++tileX)
        {
            if (IsBlockedTile(tileX, tileY))
            {
                return false;
            }
        }
    }

    return true;
}

// 유효 좌표를 보내주는 것 자체로 원본 Room 표현이 이루어짐
bool OverworldSimulation::CanPlacePlayer(std::int32_t x, std::int32_t y) const
{
    return CanPlaceBox(x, y, PlayerBoxWidth, PlayerBoxHeight);
}

bool OverworldSimulation::CanPlaceEnemy(std::int32_t x, std::int32_t y) const
{
    return CanPlaceBox(x, y, Enemy::BoxWidth, Enemy::BoxHeight);
}

bool OverworldSimulation::CanPlaceProjectile(std::int32_t x, std::int32_t y) const
{
    return CanPlaceBox(x, y, 1, 1);
}

bool OverworldSimulation::IsBlockedTile(std::int32_t tileX, std::int32_t tileY) const
{
    return _blockedTiles[tileY * MapWidth + tileX] != 0;
}

Vector2Int OverworldSimulation::GetMoveDelta(MoveDirection direction)
{
    switch (direction)
    {
    case MoveDirection::Up: return { 0, -1 };
    case MoveDirection::Down: return { 0, 1 };
    case MoveDirection::Left: return { -1, 0 };
    case MoveDirection::Right: return { 1, 0 };
    case MoveDirection::None: return { 0, 0 };
    }

    return {};
}

std::optional<ServerRoomCoordinate> OverworldSimulation::GetRoomAt(std::int32_t x, std::int32_t y) const
{
    if (x < 0 || x >= MapCellWidth || y < 0 || y >= MapCellHeight) return std::nullopt;

    return ServerRoomCoordinate(x / RoomCellWidth, y / RoomCellHeight);
}

/// <summary>
/// 현재는 A* 기반으로 계속 1 픽셀씩 이동하는 동작.
/// 나중에 적처럼 동작하려면 추적 & 공격 2가지 상태로 분리(공격 중엔 이동 X)
/// </summary>
/// <param name="enemy"></param>
void OverworldSimulation::TickEnemy(Enemy& enemy)
{
    enemy.TickAttackCooldown(); // 공격 쿨타임 계산은 원래 tick대로 계산

    const bool movable = (enemy.GetKind() == EnemyKind::Moblin) && (_tick + enemy.GetId()) % 4 == 0;  // 4틱 경과?
    const bool attackable = (enemy.GetKind() == EnemyKind::Moblin) && enemy.IsAttackReady();

    if (!movable && !attackable) return;

    // 가장 가까운 플레이어 위치 찾기
    ServerPlayerState closestPlayer;
    if (!FindClosestPlayer(enemy.GetHomeRoom(), enemy.GetPosition(), closestPlayer))
    {
        return;
    }

    if (attackable)
    {
        SpawnProjectile(enemy, closestPlayer);
        enemy.ResetAttackCooldown();
    }

    if (!movable) return;

    ServerRoomCoordinate homeRoom = enemy.GetHomeRoom();
    Vector2Int current = enemy.GetPosition();
    
    const TileCoordinate start = ToLocalTile(homeRoom, current);
    const TileCoordinate goal = ToLocalTile(homeRoom, Vector2Int(closestPlayer.x, closestPlayer.y));

    auto path = _pathfinder.FindPath(BuildNavigationGrid(homeRoom), start, goal);
    if (path.empty()) return;   // 바로 다음 칸이 goal이면 empty인 상황

    Vector2Int next = ToWorldCellPosition(homeRoom, path[0]);
    MoveDirection direction = MoveDirection::None;

    if (current.x < next.x) direction = MoveDirection::Right;
    else if (next.x < current.x) direction = MoveDirection::Left;
    else if (next.y < current.y) direction = MoveDirection::Up;
    else if (next.y > current.y) direction = MoveDirection::Down;

    Vector2Int delta = GetMoveDelta(direction);
    const Vector2Int candidate = Vector2Int(enemy.GetPosition().x + delta.x, enemy.GetPosition().y + delta.y);

    // TODO: 단순 (x, y) 검사 뿐만 아니라 box 기준으로 검사하도록 수정
    if (enemy.GetHomeRoom() != GetRoomAt(candidate.x, candidate.y)) return;
    if (!CanPlaceEnemy(candidate.x, candidate.y)) return;

    enemy.MoveTo(candidate, direction);
}

bool OverworldSimulation::FindClosestPlayer(ServerRoomCoordinate homeRoom, Vector2Int position, ServerPlayerState& closestPlayer)
{
    std::int32_t minDistance = INT32_MAX;
    for (const auto& [id, player] : _players)
    {
        if (player.dead) continue;
        auto room = GetRoomAt(player.x, player.y);
        if (!room || room != homeRoom) continue;

        Vector2Int direction = Vector2Int(player.x, player.y) - position;
        std::int32_t distance = direction.LengthSquared();
        if (distance < minDistance)
        {
            minDistance = distance;
            closestPlayer = player;
        }
    }

    return minDistance != INT32_MAX;
}

bool OverworldSimulation::TickProjectile(Projectile& projectile)
{
    if (projectile.IsExpired()) return false;

    const Vector2Int delta = GetMoveDelta(projectile.GetDirection());
    const Vector2Int candidate = projectile.GetPosition() + delta;

    if (GetRoomAt(candidate.x, candidate.y) != projectile.GetHomeRoom()) return false;
    if (!CanPlaceProjectile(candidate.x, candidate.y)) return false;

    projectile.MoveTo(candidate);
    projectile.ElapseTick();

    return !projectile.IsExpired();
}

RoomNavigationGrid OverworldSimulation::BuildNavigationGrid(ServerRoomCoordinate room) const
{
    RoomNavigationGrid grid;
    for (std::int32_t y = 0; y < RoomHeight; ++y)
    {
        for (std::int32_t x = 0; x < RoomWidth; ++x)
        {
            const TileCoordinate tile{ x, y };
            const Vector2Int world = ToWorldCellPosition(room, tile);

            grid.SetWalkable(tile, CanPlaceEnemy(world.x, world.y));
        }
    }

    return grid;
}
