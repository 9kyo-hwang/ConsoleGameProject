#include "pch.h"
#include "Game.h"
#include <Level/TitleLevel.h>
#include <Level/OverworldLevel.h>
#include <Level/GameplayLevel.h>
#include <Level/ClearLevel.h>
#include <Level/GameOverLevel.h>
#include <Level/DevelopmentLevel.h>

Game::Game()
{
    _levels.emplace_back(std::make_shared<TitleLevel>());
    _levels.emplace_back(std::make_shared<OverworldLevel>());
    _levels.emplace_back(std::make_shared<GameplayLevel>());
    _levels.emplace_back(std::make_shared<ClearLevel>());
    _levels.emplace_back(std::make_shared<GameOverLevel>());
    _levels.emplace_back(std::make_shared<DevelopmentLevel>());

    SetSubLevel(_levels[(int)_state]);
}

void Game::ChangeLevel(State state)
{
    if (_state == state)
    {
        return;
    }

    if (state == State::Overworld)
    {
        // 사망한 캐릭터를 새로 생성해야 함
        _levels[(int)State::Overworld] = std::make_shared<OverworldLevel>();
    }

    _state = state;
    SetSubLevel(_levels[(int)state]);
}