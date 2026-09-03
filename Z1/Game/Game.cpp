#include "pch.h"
#include "Game.h"
#include <Level/TitleLevel.h>
#include <Level/OverworldLevel.h>
#include <Level/ClearLevel.h>
#include <Level/GameOverLevel.h>
#include <Level/DevelopmentLevel.h>
#include <Level/CaveLevel.h>
#include <Level/DungeonLevel.h>
#include <Actor/Player.h>
#include <Sockets/Endpoint.h>

using namespace Net;
using namespace Z1::Protocol;

Game::Game()
{
    _levels.resize((size_t)State::END);

    _levels[(int)State::Title] = (std::make_shared<TitleLevel>());
    _levels[(int)State::Overworld] = (std::make_shared<OverworldLevel>());
    _levels[(int)State::SwordCave] = (std::make_shared<CaveLevel>());
    _levels[(int)State::Dungeon1] = (std::make_shared<DungeonLevel>());
    _levels[(int)State::Clear] = (std::make_shared<ClearLevel>());
    _levels[(int)State::GameOver] = (std::make_shared<GameOverLevel>());
    _levels[(int)State::Development] = (std::make_shared<DevelopmentLevel>());

    SetSubLevel(_levels[(int)_state]);
}

void Game::StartNewGame()
{
    ResetPlayerState();

    ConnectToServer();  // 일단 Overworld 로 제한했으니, 게임 세션 시작할 때 서버 연결도 한 번 시도하자.

    _levels[(int)State::Overworld] = std::make_shared<OverworldLevel>();
    _levels[(int)State::SwordCave] = std::make_shared<CaveLevel>();
    _levels[(int)State::Dungeon1] = std::make_shared<DungeonLevel>();

    ChangeLevel(State::Overworld);
}

void Game::ResetPlayerState()
{
    _playerState = PlayerState{};
}

void Game::SavePlayerState(const Player& player)
{
    _playerState.hp = std::clamp(player.GetHp(), 0, PlayerMaxHp);
    _playerState.hasSword = player.HasSword();
}

void Game::LoadPlayerState(Player& player) const
{
    player.SetHealth(_playerState.hp);

    if (_playerState.hasSword)
    {
        player.EquipSword();
    }
}

bool Game::ConnectToServer()
{
    if (_network.IsConnected())
    {
        return true;
    }

    Endpoint endpoint = Endpoint::Loopback(7777);
    if (!_network.Start(endpoint))
    {
        std::cout << "Failed to connect Z1Server: "
            << _network.GetLastError()
            << "\n";

        // TODO: 연결 실패 시 싱글 플레이로 돌아가도록 하기?
        return false;
    }

    return true;
}

// Only MainThread
void Game::PumpNetwork()
{
    IncomingMessage message;
    while (_network.TryPopIncomingMessage(message))
    {
        std::visit([this](const auto& received)
            {
                using T = std::decay_t<decltype(received)>; // 참조, const, volatile 한정자를 제거하고 순수 값타입으로 바꾸는 역할

                if constexpr (std::is_same_v<T, EnterMessage>)
                {
                    _localPlayerId = received.playerId;
                }
                else if constexpr (std::is_same_v<T, WorldSnapshot>)
                {
                    _latestSnapshot = received;
                }
                else if constexpr (std::is_same_v<T, CombatEvent>)
                {
                    _pendingCombatEvents.emplace_back(received);
                }
                else if constexpr (std::is_same_v<T, EnemyPathDebug>)
                {
                    _latestEnemyPaths[received.id] = received;
                }
            }, message);
    }
}

void Game::ChangeLevel(State state)
{
    if (_state == state)
    {
        return;
    }

    //if (state == State::Overworld)
    //{
    //    // 사망한 캐릭터를 새로 생성해야 함
    //    _levels[(int)State::Overworld] = std::make_shared<OverworldLevel>();
    //}

    _state = state;
    SetSubLevel(_levels[(int)state]);
}
