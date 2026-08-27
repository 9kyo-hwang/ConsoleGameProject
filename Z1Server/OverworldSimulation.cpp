#include "pch.h"
#include "OverworldSimulation.h"

namespace
{
    constexpr std::int32_t PlayerMoveCellsPerTick = 1;
    
    // 클라쪽 크기를 일단 가져왔는데...
    constexpr std::int32_t PlayerBoxWidth = 8;
    constexpr std::int32_t PlayerBoxHeight = 5;
    
    constexpr std::int32_t TileCellWidth = 10;
    constexpr std::int32_t TileCellHeight = 5;
    
    constexpr std::int32_t RoomTileWidth = 16;
    constexpr std::int32_t RoomTileHeight = 11;
    
    constexpr std::int32_t OverworldRoomColumns = 16;
    constexpr std::int32_t OverworldRoomRows = 8;

    constexpr std::int32_t WorldWidth = OverworldRoomColumns * RoomTileWidth * TileCellWidth;
    constexpr std::int32_t WorldHeight = OverworldRoomRows * RoomTileHeight * TileCellHeight;
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

void OverworldSimulation::Tick()
{
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

        const MoveDelta delta = GetMoveDelta(direction);
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
}

bool OverworldSimulation::CanPlacePlayer(std::int32_t x, std::int32_t y)
{
    // 일단 OverworldMap 밖에 안나가는지 검사
    return x >= 0 && y >= 0 && x + PlayerBoxWidth <= WorldWidth && y + PlayerBoxHeight <= WorldHeight;
}

MoveDelta OverworldSimulation::GetMoveDelta(Z1::Protocol::MoveDirection direction)
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
