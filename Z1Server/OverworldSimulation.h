#pragma once

#include <optional>
#include <unordered_map>
#include <filesystem>
#include <string>
#include <vector>

struct MoveDelta
{
    std::int32_t x = 0;
    std::int32_t y = 0;
};

struct ServerRoomCoordinate
{
    std::int32_t x = 0;
    std::int32_t y = 0;

    auto operator<=>(const ServerRoomCoordinate&) const = default;
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
    };

    struct ServerEnemyState
    {
        std::uint32_t id = 0;
        Z1::Protocol::EnemyKind kind = Z1::Protocol::EnemyKind::Octorok;

        ServerRoomCoordinate home;
        std::int32_t spawnX = 0;
        std::int32_t spawnY = 0;

        std::int32_t x = 0;
        std::int32_t y = 0;
        Z1::Protocol::MoveDirection facing = Z1::Protocol::MoveDirection::Up;
        std::int32_t hp = 1;
        bool dead = false;
    };

public:
    // 기존 Z1의 OverworldMap - BlockingMap 파싱 파트만 차용
    bool LoadBlockingMap(const FilePath& path, std::string& error);
    bool SpawnEnemies(std::uint32_t seed); // Map 로드 시 적 배치

    bool AddPlayer(std::uint32_t playerId);
    void RemovePlayer(std::uint32_t playerId);

    // false: 플레이어 없음 / true: 입력 갱신(오래된 입력 무시)
    bool SetInput(std::uint32_t playerId, const Z1::Protocol::InputCommand& input);

    // Session이나 packet을 모른 채, 현재 월드 상태를 Snapshot 용 값으로 복사해주는 API를 제공.
    WorldSnapshot BuildPlayerSnapshot(std::uint32_t id, std::uint32_t tick);

    void Tick();

private:
    void TickEnemy(ServerEnemyState& enemy);

    // 클라의 CanPlaceBox류 충돌맵 검사에 대응
    bool CanPlaceBox(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) const;
    bool CanPlacePlayer(std::int32_t x, std::int32_t y) const;
    bool IsBlockedTile(std::int32_t x, std::int32_t y) const;

    MoveDelta GetMoveDelta(Z1::Protocol::MoveDirection direction);
    std::optional<ServerRoomCoordinate> GetRoomAt(std::int32_t x, std::int32_t y) const;

private:
    // id - state
    std::unordered_map<std::uint32_t, ServerPlayerState> _players;
    std::unordered_map<std::uint32_t, ServerEnemyState> _enemies;
    std::uint32_t _enemyId = 1;

    std::vector<std::uint8_t> _blockedTiles;
    bool _hasBlockingMap = false;
};

