#pragma once

#include <Types.h>
#include <optional>
#include <unordered_map>
#include <filesystem>
#include <string>
#include <vector>
#include <Actor/Player.h>
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
    bool LoadBlockingMap(const FilePath& path, std::string& error);

    // 기존 Z1의 OverworldMap - BlockingMap 파싱 파트만 차용
    std::optional<std::uint32_t> SpawnPlayer();
    void DespawnPlayer(std::uint32_t playerId);
    
    bool SpawnEnemies(std::uint32_t seed); // Map 로드 시 적 배치
    bool SpawnProjectile(Enemy& enemy, const Player& target);    // 일단 moblin만

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

    void Tick(float deltaTime);

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
    bool FindClosestPlayer(ServerRoomCoordinate homeRoom, Vector2Int position, const Player*& closestPlayer);
    bool TickProjectile(Projectile& projectile);

    bool TryHitPlayer(const Projectile& projectile, Vector2Int candidate);
    bool TryHitEnemy(Player& player);    // 플레이어 근접 검 공격
    bool TryRespawnEnemy(Enemy& enemy);

    void RecordEnemyChasePath(const Enemy& enemy, const std::vector<TileCoordinate>& path);

private:
    RoomNavigationGrid BuildNavigationGrid(ServerRoomCoordinate room) const;

private:
    // id - state
    std::unordered_map<std::uint32_t, Player> _players;
    std::unordered_map<std::uint32_t, Enemy> _enemies;
    std::unordered_map<std::uint32_t, Projectile> _projectiles;

    std::vector<std::uint8_t> _blockedTiles;
    bool _hasBlockingMap = false;
    std::uint32_t _tick = 0;

    std::vector<PendingCombatEvent> _pendingCombatEvents;
    std::unordered_map<std::uint32_t, Z1::Protocol::EnemyPathDebug> _dbgPaths;
};
