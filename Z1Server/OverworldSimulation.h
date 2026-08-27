#pragma once

#include <optional>
#include <unordered_map>

struct MoveDelta
{
    std::int32_t x = 0;
    std::int32_t y = 0;
};

// Z1의 OverworldMap에서 컨트롤하는 충돌 판정, Room, 적/투사체 등의 서버 버전
class OverworldSimulation
{
public:
    bool AddPlayer(std::uint32_t playerId);
    void RemovePlayer(std::uint32_t playerId);

    // false: 플레이어 없음 / true: 입력 갱신(오래된 입력 무시)
    bool SetInput(std::uint32_t playerId, const Z1::Protocol::InputCommand& input);

    void Tick();

private:
    // 클라의 CanPlaceBox류 충돌맵 검사에 대응
    bool CanPlacePlayer(std::int32_t x, std::int32_t y);
    MoveDelta GetMoveDelta(Z1::Protocol::MoveDirection direction);

private:
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

    // id - state
    std::unordered_map<std::uint32_t, ServerPlayerState> _players;
};

