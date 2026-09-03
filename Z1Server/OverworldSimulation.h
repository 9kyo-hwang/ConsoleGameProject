#pragma once

#include <Types.h>
#include <optional>
#include <unordered_map>
#include <filesystem>
#include <string>
#include <vector>
#include <Actor/Enemy.h>
#include <Actor/Projectile.h>
#include <array>
#include <RoomPathfinder.h>

// PlayerSwordAttack 공격 시도는 Simulation에서 발생해서,
// 여기서 아래 구조체의 Queue에 밀어넣고, Server에서 꺼내쓰자.
struct PendingCombatEvent
{
    Z1::Protocol::CombatEvent event;
    ServerRoomCoordinate room;
};

// Z1의 OverworldMap에서 컨트롤하는 충돌 판정, Room, 적/투사체 등의 서버 버전
class OverworldSimulation
{
    using FilePath = std::filesystem::path;

public:
    struct ServerPlayerState
    {
        std::uint32_t playerId = 0;

        // Server 시뮬에 사용되는 다른 좌표(단위는 동일)
        std::int32_t x = 0;
        std::int32_t y = 0;

        Z1::Protocol::MoveDirection facing = Z1::Protocol::MoveDirection::Up;

        std::int32_t hp = 20;
        bool dead = false;

        Z1::Protocol::InputCommand latestInput{};
        std::optional<std::uint32_t> lastInputSequence;
        bool attackRequested = false;   // 다음 입력이 올 때까지 flag가 유지되어, '1회'만 발동하도록 플래그
    };

public:
    // 기존 Z1의 OverworldMap - BlockingMap 파싱 파트만 차용
    bool LoadBlockingMap(const FilePath& path, std::string& error);
    bool SpawnEnemies(std::uint32_t seed); // Map 로드 시 적 배치
    bool SpawnProjectile(Enemy& enemy, const ServerPlayerState& target);    // 일단 moblin만

    bool AddPlayer(std::uint32_t playerId);
    void RemovePlayer(std::uint32_t playerId);

    // false: 플레이어 없음 / true: 입력 갱신(오래된 입력 무시)
    bool SetInput(std::uint32_t playerId, const Z1::Protocol::InputCommand& input);

    // Session이나 packet을 모른 채, 현재 월드 상태를 Snapshot 용 값으로 복사해주는 API를 제공.
    Z1::Protocol::WorldSnapshot BuildSnapshot(std::uint32_t id);

    inline std::vector<PendingCombatEvent> TakeCombatEvents()
    {
        return std::exchange(_pendingCombatEvents, std::vector<PendingCombatEvent>());
    }

    std::vector<Z1::Protocol::EnemyPathDebug> BuildEnemyPathDebug(std::uint32_t playerId);

    bool IsPlayerInRoom(std::uint32_t playerId, ServerRoomCoordinate room);

    void Tick();
    inline std::uint32_t GetTick() const noexcept { return _tick; }

private:
    // 클라의 CanPlaceBox류 충돌맵 검사에 대응
    bool CanPlaceBox(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) const;
    bool CanPlacePlayer(std::int32_t x, std::int32_t y) const;
    bool CanPlaceEnemy(std::int32_t x, std::int32_t y) const;
    bool CanPlaceProjectile(std::int32_t x, std::int32_t y) const;
    bool IsBlockedTile(std::int32_t x, std::int32_t y) const;

    Vector2Int GetMoveDelta(Z1::Protocol::MoveDirection direction);
    std::optional<ServerRoomCoordinate> GetRoomAt(std::int32_t x, std::int32_t y) const;

    void TickEnemy(Enemy& enemy);
    bool FindClosestPlayer(ServerRoomCoordinate homeRoom, Vector2Int position, ServerPlayerState& closestPlayer);

    bool TickProjectile(Projectile& projectile);
    bool TryHitPlayer(const Projectile& projectile, Vector2Int candidate);
    bool TryHitEnemy(ServerPlayerState& player);    // 플레이어 근접 검 공격

    void RecordEnemyChasePath(const Enemy& enemy, const std::vector<TileCoordinate>& path);

private:
    RoomNavigationGrid BuildNavigationGrid(ServerRoomCoordinate room) const;
    RoomPathfinder _pathfinder;

private:
    // id - state
    std::unordered_map<std::uint32_t, ServerPlayerState> _players;
    std::unordered_map<std::uint32_t, Enemy> _enemies;
    std::unordered_map<std::uint32_t, Projectile> _projectiles;
    std::uint32_t _enemyId = 1;
    std::uint32_t _projectileId = 1;

    std::vector<std::uint8_t> _blockedTiles;
    bool _hasBlockingMap = false;
    std::uint32_t _tick = 0;

    std::vector<PendingCombatEvent> _pendingCombatEvents;
    std::unordered_map<std::uint32_t, Z1::Protocol::EnemyPathDebug> _dbgPaths;
};

