#include "pch.h"
#include "OverworldSimulation.h"
#include <algorithm>
#include <fstream>
#include <array>
#include <random>
#include <utility>

using namespace Z1::Protocol;

namespace
{
    constexpr std::uint16_t PlayerMoveCellsPerTick = 1;
    
    // 클라쪽 크기를 일단 가져왔는데...
    constexpr std::uint16_t PlayerBoxWidth = 8;
    constexpr std::uint16_t PlayerBoxHeight = 5;
    constexpr std::uint16_t EnemiesPerRoom = 6;
    constexpr std::uint16_t MaxSpawnAttempts = 30;

    std::uint32_t MakeRoomSeed(ServerRoomCoordinate room, std::uint32_t seed)
    {
        return seed ^ (std::uint32_t)room.x * 73856093u ^ (std::uint32_t)room.y * 19349663u;
    }
    
    // 타일 하나를 구성하는 셀
    constexpr std::uint16_t TileCellWidth = 10;
    constexpr std::uint16_t TileCellHeight = 5;
    
    // Room 하나를 구성하는 논리적 칸 개수
    constexpr std::uint16_t RoomWidth = 16;
    constexpr std::uint16_t RoomHeight = 11;

    // Room 하나를 구성하는 셀
    constexpr std::uint16_t RoomCellWidth = RoomWidth * TileCellWidth;
    constexpr std::uint16_t RoomCellHeight = RoomHeight * TileCellHeight;
    
    // Map을 구성하는 Room 개수
    constexpr std::uint16_t OverworldRoomColumns = 16;
    constexpr std::uint16_t OverworldRoomRows = 8;

    // Map을 구성하는 논리적 칸 개수
    constexpr std::uint16_t MapWidth = OverworldRoomColumns * RoomWidth;
    constexpr std::uint16_t MapHeight = OverworldRoomRows * RoomHeight;

    // Map을 구성하는 셀 칸 개수
    constexpr std::uint16_t MapCellWidth = MapWidth * TileCellWidth;
    constexpr std::uint16_t MapCellHeight = MapHeight * TileCellHeight;

    const ServerRoomCoordinate StartRoom{ 7, 7 };

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

/*
* Tile: (10, 5) / Room: (16, 11)
* StartRoom: (7, 7) / LocalSpawn: (7, 2)
* 따라서 전체 Map 좌표 기준으로 (16, 11) x (10, 5) x (7, 7) + (7, 2) * (10, 5) = (1190, 395)
*/
std::optional<std::uint32_t> OverworldSimulation::SpawnPlayer()
{
    Player player;  // 1190, 395 기본 세팅

    std::uint32_t id = player.GetId();
    const auto [it, inserted] = _players.emplace(id, std::move(player));

    return inserted ? std::optional(id) : std::nullopt;
}

void OverworldSimulation::DespawnPlayer(std::uint32_t playerId)
{
    // 삭제할 id가 없으면 0 반환
    _players.erase(playerId);
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
            DistInt enemyKind((std::int32_t)ActorKind::Enemy_Octorok, (std::int32_t)ActorKind::Enemy_Tektite);

            std::int32_t spawned = 0;
            for (std::int32_t attempt = 0; attempt < MaxSpawnAttempts && spawned < EnemiesPerRoom; ++attempt)
            {
                std::int32_t x = room.x * RoomCellWidth + localTileX(rng) * TileCellWidth;
                std::int32_t y = room.y * RoomCellHeight + localTileY(rng) * TileCellHeight;
                if (!CanPlaceBox(x, y, Enemy::BoxWidth, Enemy::BoxHeight))
                {
                    continue;
                }

                ActorKind kind = (ActorKind)enemyKind(rng);
                Vector2Int spawnPosition(x, y);

                Enemy enemy(kind, room, spawnPosition);
                _enemies.emplace(enemy.GetId(), std::move(enemy));
                ++spawned;
            }
        }
    }

    return true;
}

bool OverworldSimulation::SpawnProjectile(Enemy& enemy, const Player& target)
{
    const Vector2Int targetPos = Vector2Int(target.GetPosition().x, target.GetPosition().y);
    const MoveDirection direction = GetDirectionToward(enemy.GetPosition(), targetPos);
    if (direction == MoveDirection::None) return false;

    const Vector2Int spawnPos = enemy.GetProjectileSpawnPosition();
    if (GetRoomAt(spawnPos.x, spawnPos.y) != enemy.GetHomeRoom()) return false;
    if (!CanPlaceProjectile(spawnPos.x, spawnPos.y)) return false;

    Projectile projectile(ActorKind::Projectile_Spear, enemy.GetId(), enemy.GetHomeRoom(), spawnPos, direction, 1, 40);

    return _projectiles.emplace(projectile.GetId(), std::move(projectile)).second;
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

    Player& player = found->second;
    if (player.HasInputted() && input.sequence <= player.LastInputSequence())
    {
        // TCP는 순서를 보장해서, 더 오래된 입력 시퀀스가 들어오는 케이스는 보통 없음
        // 중복되거나 오래된 입력은 무시
        std::cout << "[C2S_Input] ignored: player=" << playerId << ", sequence=" << input.sequence << "\n";
        return true;
    }

    player.UpdateInput(input);  // 여기서 actionFlag = 0으로 세팅: 1회만 적용되도록

    std::cout << "[C2S_Input] player=" << playerId
        << ", sequence=" << input.sequence
        << ", direction=" << (int)input.moveDirection
        << ", actions=" << (int)input.actionFlags
        << ", position=" << player.GetPosition().x << ", " << player.GetPosition().y << ")\n";
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
    const auto room = GetRoomAt(myPlayer.GetPosition().x, myPlayer.GetPosition().y);
    if (!room) return snapshot;

    auto& actors = snapshot.actors;

    for (const auto& [id, player] : _players)
    {
        if (GetRoomAt(player.GetPosition().x, player.GetPosition().y) == room)
        {
            actors.push_back(player.GetInfo());
        }
    }

    for (const auto& [id, enemy] : _enemies)
    {
        if (!enemy.IsDead() && enemy.GetHomeRoom() == *room)
        {
            actors.push_back(enemy.GetInfo());
        }
    }

    for (const auto& [id, projectile] : _projectiles)
    {
        if (!projectile.IsExpired() && projectile.GetHomeRoom() == *room)
        {
            actors.push_back(projectile.GetInfo());
        }
    }

    std::sort(actors.begin(), actors.end(),
        [](const ActorInfo& lhs, const ActorInfo& rhs)
        {
            return lhs.id < rhs.id;
        });

    return snapshot;
}

std::vector<EnemyPathDebug> OverworldSimulation::BuildEnemyPathDebug(std::uint32_t playerId)
{
    auto found = _players.find(playerId);
    if (found == _players.end())
    {
        return {};
    }

    const auto& player = found->second;
    const auto room = GetRoomAt(player.GetPosition().x, player.GetPosition().y);
    if (!room) return {};

    std::vector<EnemyPathDebug> dbgPaths;
    dbgPaths.reserve(_dbgPaths.size());

    for (const auto& [enemyId, dbgPath] : _dbgPaths)
    {
        auto it = _enemies.find(enemyId);
        if (it == _enemies.end()) continue;

        const Enemy& enemy = it->second;    // 기존 [] 연산자로 꺼내오면 기본 생성자 호출을 시도해서 에러
        const ServerRoomCoordinate enemyRoom{ dbgPath.roomX, dbgPath.roomY };
        if (enemyRoom != room)  // IsDead() 검사 제외: 죽은 적에 대한 empty paths를 보내야 함
        {
            continue;
        }

        dbgPaths.push_back(dbgPath);
    }

    sort(dbgPaths.begin(), dbgPaths.end(),
        [](const EnemyPathDebug& lhs, const EnemyPathDebug& rhs)
        {
            return lhs.id < rhs.id;
        });

    return dbgPaths;
}

bool OverworldSimulation::IsPlayerInRoom(std::uint32_t playerId, ServerRoomCoordinate room)
{
    auto found = _players.find(playerId);
    if (found == _players.end()) return false;

    const Player& player = found->second;
    return GetRoomAt(player.GetPosition().x, player.GetPosition().y) == room;
}

void OverworldSimulation::Tick()
{
    ++_tick;

    // 1. 플레이어 위치 갱신하고
    for (auto& [id, player] : _players)
    {
        if (player.IsDead())
        {
            continue;
        }

        const bool attackRequested = player.PerformAttackRequest();
        const MoveDirection direction = player.GetInputDirection();
        if (direction != MoveDirection::None)
        {
            // 입력이 있으면 막히더라도 방향은 바뀜
            player.SetDirection(direction);

            const Vector2Int delta = GetMoveDelta(direction);
            for (std::int32_t step = 0; step < PlayerMoveCellsPerTick; ++step)
            {
                const std::int32_t candidateX = player.GetPosition().x + delta.x;
                const std::int32_t candidateY = player.GetPosition().y + delta.y;

                if (CanPlacePlayer(candidateX, candidateY))
                {
                    player.SetPosition(candidateX, candidateY);
                }
            }
        }
        else if (attackRequested)   // 이동 중이 아니면서 공격 요청이 들어왔다면
        {
            // 공격 성공 유무와 관계없이, 이벤트는 무조건 발동
            auto room = GetRoomAt(player.GetPosition().x, player.GetPosition().y);
            if (!room) continue;

            PendingCombatEvent event
            {
                .event = CombatEvent{CombatEventType::PlayerSwordAttack, player.GetId(), player.GetDirection()},
                .room = *room
            };

            _pendingCombatEvents.emplace_back(event);

            TryHitEnemy(player);
        }
    }

    // 2. player가 위치한 Room 받아오고
    std::array<bool, OverworldRoomColumns * OverworldRoomRows> activeRooms{};
    for (const auto& [id, player] : _players)
    {
        if (player.IsDead()) continue;

        if (const auto room = GetRoomAt(player.GetPosition().x, player.GetPosition().y))
        {
            ServerRoomCoordinate r = *room;
            activeRooms[r.y * OverworldRoomColumns + r.x] = true;
        }
    }

    // 3. 적 목록에서 activeRoom에 속한 것들 활성화
    for (auto& [id, enemy] : _enemies)
    {
        if (enemy.IsDead())
        {
            TryRespawnEnemy(enemy);
        }

        // 적이 죽었거나, 현재 활성 Room이 아니라면 경로 표시 안하도록 기록을 Clear
        const ServerRoomCoordinate homeRoom = enemy.GetHomeRoom();
        if (enemy.IsDead() || !activeRooms[homeRoom.y * OverworldRoomColumns + homeRoom.x])
        {
            if (_dbgPaths.contains(id))
            {
                RecordEnemyChasePath(enemy, {});
            }
            
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

    const bool movable = (enemy.GetKind() == ActorKind::Enemy_Moblin) && (_tick + enemy.GetId()) % 4 == 0;
    const bool attackable = (enemy.GetKind() == ActorKind::Enemy_Moblin) && enemy.IsAttackReady();

    // 가장 가까운 플레이어 위치 찾기
    const Player* closestPlayer = nullptr;
    if (!FindClosestPlayer(enemy.GetHomeRoom(), enemy.GetPosition(), closestPlayer))
    {
        // 빈 경로
        RecordEnemyChasePath(enemy, {});
        return;
    }

    // 일단 경로는 시도(빈 경로 기록을 위해)
    if (!movable && !attackable) return;

    if (attackable)
    {
        SpawnProjectile(enemy, *closestPlayer);
        enemy.ResetAttackCooldown();
    }

    if (!movable) return;

    if (enemy.IsMoving())   // 이전에 계산된 waypoint가 있다면
    {
        const Vector2Int current = enemy.GetPosition();
        Vector2Int delta = GetMoveDelta(enemy.GetDirection());
        const Vector2Int candidate = Vector2Int(current.x + delta.x, current.y + delta.y);

        // 만약 다음 픽셀 칸으로 이동 안되면 Clear
        if (enemy.GetHomeRoom() != GetRoomAt(candidate.x, candidate.y) ||
            !CanPlaceEnemy(candidate.x, candidate.y))
        {
            enemy.ClearNextWaypoint();
            return;
        }

        enemy.SetPosition(candidate.x, candidate.y);    // nextWaypoint에 도달하면 안에서 reset됨
        return;
    }

    ServerRoomCoordinate homeRoom = enemy.GetHomeRoom();
    Vector2Int current = enemy.GetPosition();
    
    const TileCoordinate start = ToLocalTile(homeRoom, current);
    const TileCoordinate goal = ToLocalTile(homeRoom, Vector2Int(closestPlayer->GetPosition().x, closestPlayer->GetPosition().y));

    auto path = RoomPathfinder::FindPath(BuildNavigationGrid(homeRoom), start, goal);
    RecordEnemyChasePath(enemy, path);
    if (path.empty()) return;   // 바로 다음 칸이 goal이면 empty인 상황

    Vector2Int next = ToWorldCellPosition(homeRoom, path[0]);
    
    // TODO: 단순 (x, y) 검사 뿐만 아니라 box 기준으로 검사하도록 수정
    if (enemy.GetHomeRoom() != GetRoomAt(next.x, next.y)) return;
    if (!CanPlaceEnemy(next.x, next.y)) return;

    MoveDirection direction = MoveDirection::None;
    if (current.x < next.x) direction = MoveDirection::Right;
    else if (next.x < current.x) direction = MoveDirection::Left;
    else if (next.y < current.y) direction = MoveDirection::Up;
    else if (next.y > current.y) direction = MoveDirection::Down;

    Vector2Int delta = GetMoveDelta(direction);
    const Vector2Int candidate = Vector2Int(enemy.GetPosition().x + delta.x, enemy.GetPosition().y + delta.y);

    enemy.SetNextWaypoint(next.x, next.y);
    enemy.SetDirection(direction);
    enemy.SetPosition(candidate.x, candidate.y);
}

bool OverworldSimulation::FindClosestPlayer(ServerRoomCoordinate homeRoom, Vector2Int position, const Player*& closestPlayer)
{
    std::int32_t minDistance = INT32_MAX;
    for (const auto& [id, player] : _players)
    {
        if (player.IsDead()) continue;
        auto room = GetRoomAt(player.GetPosition().x, player.GetPosition().y);
        if (!room || room != homeRoom) continue;

        Vector2Int direction = Vector2Int(player.GetPosition().x, player.GetPosition().y) - position;
        std::int32_t distance = direction.LengthSquared();
        if (distance < minDistance)
        {
            minDistance = distance;
            closestPlayer = &player;
        }
    }

    return minDistance != INT32_MAX;
}

// 충돌 유무
bool OverworldSimulation::TickProjectile(Projectile& projectile)
{
    if (projectile.IsExpired()) return false;

    const auto owner = _enemies.find(projectile.GetOwnerId());
    if (owner == _enemies.end() || owner->second.IsDead())
    {
        return false;
    }

    const Vector2Int delta = GetMoveDelta(projectile.GetDirection());
    const Vector2Int candidate = projectile.GetPosition() + delta;

    if (GetRoomAt(candidate.x, candidate.y) != projectile.GetHomeRoom()) return false;
    if (!CanPlaceProjectile(candidate.x, candidate.y)) return false;

    if (TryHitPlayer(projectile, candidate))
    {
        return false;
    }

    projectile.SetPosition(candidate.x, candidate.y);
    projectile.ElapseTick();

    return !projectile.IsExpired();
}

bool OverworldSimulation::TryHitPlayer(const Projectile& projectile, Vector2Int candidate)
{
    std::int32_t damage = projectile.GetDamage();
    if (damage <= 0) return false;

    const std::int32_t left = candidate.x;
    const std::int32_t top = candidate.y;
    const std::int32_t right = left + 1;   // 투사체는 폭이 1
    const std::int32_t bottom = top + 1;

    for (auto& [id, player] : _players)
    {
        if (player.IsDead()) continue;

        const Vector2Int playerPos = player.GetPosition();

        auto room = GetRoomAt(playerPos.x, playerPos.y);
        if (!room || *room != projectile.GetHomeRoom())
        {
            continue;
        }

        const std::int32_t playerLeft = playerPos.x;
        const std::int32_t playerTop = playerPos.y;
        const std::int32_t playerRight = playerLeft + PlayerBoxWidth;
        const std::int32_t playerBottom = playerTop + PlayerBoxHeight;

        const bool overlaps =
            left < playerRight && playerLeft < right &&
            top < playerBottom && playerTop < bottom;

        if (!overlaps) continue;

        player.TakeDamage(damage);
        return true;
    }

    return false;
}

bool OverworldSimulation::TryHitEnemy(Player& player)
{
    constexpr std::int32_t HorizontalSwordWidth = 10;
    constexpr std::int32_t HorizontalSwordHeight = 3;
    constexpr std::int32_t VerticalSwordWidth = 6;
    constexpr std::int32_t VerticalSwordHeight = 5;

    // 플레이어: 8 x 5 
    // -> 좌우에서 Top 기준 + 1하면 중앙으로 맞춰짐
    // -> 상하에서 Left 기준 + 1하면 중앙으로 맞춰짐

    // 검 공격 영역의 Left/Top 및 Width/Height
    std::int32_t left = 0;
    std::int32_t top = 0;
    std::int32_t width = 0;
    std::int32_t height = 0;

    Vector2Int playerPos = player.GetPosition();

    switch (player.GetDirection())
    {
    case MoveDirection::Right:
    {
        left = playerPos.x + PlayerBoxWidth;
        top = playerPos.y + 1;
        width = HorizontalSwordWidth;
        height = HorizontalSwordHeight;
        break;
    }
    case MoveDirection::Left:
    {
        left = playerPos.x - HorizontalSwordWidth;
        top = playerPos.y + 1;
        width = HorizontalSwordWidth;
        height = HorizontalSwordHeight;
        break;
    }
    case MoveDirection::Up:
    {
        left = playerPos.x + 1;
        top = playerPos.y - VerticalSwordHeight;
        width = VerticalSwordWidth;
        height = VerticalSwordHeight;
        break;
    }
    case MoveDirection::Down:
    {
        left = playerPos.x + 1;
        top = playerPos.y + PlayerBoxHeight;
        width = VerticalSwordWidth;
        height = VerticalSwordHeight;
        break;
    }
    default: return false;
    }

    const auto room = GetRoomAt(playerPos.x, playerPos.y);
    if (!room) return false;

    Enemy* target = nullptr;
    for (auto& [id, enemy] : _enemies)
    {
        if (enemy.IsDead() || enemy.GetHomeRoom() != *room)
        {
            continue;
        }

        Vector2Int enemyPos = enemy.GetPosition();
        bool overlaps =
            left < enemyPos.x + Enemy::BoxWidth &&
            enemyPos.x < left + width &&
            top < enemyPos.y + Enemy::BoxHeight &&
            enemyPos.y < top + height;

        if (!overlaps) continue;

        // 하나의 적만 공격하도록 ID 가장 작은 것만.
        if (!target || enemy.GetId() < target->GetId())
        {
            target = &enemy;
        }
    }

    if (!target) return false;

    target->TakeDamage(1, _tick);
    return true;
}

bool OverworldSimulation::TryRespawnEnemy(Enemy& enemy)
{
    if (!enemy.CanRespawn(_tick))
    {
        return false;
    }

    // 사실 SpawnPosition CanPlaceEnemy()는 필요 없음. 이미 생성 당시 검사 통과한 좌표이기 때문에.
    const Vector2Int spawnPosition = enemy.GetSpawnPosition();
    for (const auto& [id, player] : _players)
    {
        if (player.IsDead()) continue;

        const Vector2Int playerPos = player.GetPosition();

        const bool overlaps =
            spawnPosition.x < playerPos.x + PlayerBoxWidth &&
            spawnPosition.y < playerPos.y + PlayerBoxHeight &&
            playerPos.x < spawnPosition.x + Enemy::BoxWidth &&
            playerPos.y < spawnPosition.y + Enemy::BoxHeight;

        if (overlaps) return false;
    }

    enemy.Respawn();
    return true;
}

void OverworldSimulation::RecordEnemyChasePath(const Enemy& enemy, const std::vector<TileCoordinate>& path)
{
    EnemyPathDebug dbg;
    dbg.tick = _tick;
    dbg.id = enemy.GetId();
    dbg.roomX = enemy.GetHomeRoom().x;
    dbg.roomY = enemy.GetHomeRoom().y;
    dbg.tileIndices.reserve(path.size());

    for (TileCoordinate tile : path)
    {
        dbg.tileIndices.push_back(tile.y * 16 + tile.x);
    }

    _dbgPaths[enemy.GetId()] = dbg;
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
