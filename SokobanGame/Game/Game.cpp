#include <pch.h>
#include "Game.h"

#include <Level/GameLevel.h>
#include <Level/MenuLevel.h>

Game::Game()
{
    _levels.emplace_back(std::make_shared<GameLevel>());
    _levels.emplace_back(std::make_shared<MenuLevel>());

    // 지정한(게임플레이 = 0, 메뉴 = 1) 상태를 메인 레벨로
    _state = State::Gameplay;
    mainLevel = _levels[(int)_state];
}

void Game::ToggleMenu()
{
    // 0 <-> 1
    int stateIndex = (int)_state;
    int nextStateIndex = 1 - stateIndex;

    _state = (State)nextStateIndex;
    SetSubLevel(_levels[nextStateIndex]);
}
