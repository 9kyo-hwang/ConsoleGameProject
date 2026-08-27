#include "pch.h"
#include "OverworldSimulation.h"

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
        << "\n";
    return true;
}

void OverworldSimulation::Tick()
{
}
