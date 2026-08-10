#include "pch.h"
#include "Game.h"
#include <Level/TitleLevel.h>
#include <Level/GameplayLevel.h>
#include <Level/ClearLevel.h>

Game::Game()
{
    _levels.emplace_back(std::make_shared<TitleLevel>());
    _levels.emplace_back(std::make_shared<GameplayLevel>());
    _levels.emplace_back(std::make_shared<ClearLevel>());

    SetSubLevel(_levels[(int)_state]);
}

void Game::ChangeLevel(State state)
{
    if (_state == state)
    {
        return;
    }

    _state = state;
    SetSubLevel(_levels[(int)state]);
}