#include "pch.h"
#include "Game.h"
#include <Level/TitleLevel.h>
#include <Level/OverworldLevel.h>
#include <Level/GameplayLevel.h>
#include <Level/ClearLevel.h>
#include <Level/GameOverLevel.h>
#include <Level/DevelopmentLevel.h>
#include <Level/CaveLevel.h>
#include <Level/DungeonLevel.h>

Game::Game()
{
    _levels.resize((size_t)State::END);

    _levels[(int)State::Title] = (std::make_shared<TitleLevel>());
    _levels[(int)State::Overworld] = (std::make_shared<OverworldLevel>());
    _levels[(int)State::SwordCave] = (std::make_shared<CaveLevel>());
    _levels[(int)State::Dungeon1] = (std::make_shared<DungeonLevel>());
    _levels[(int)State::Gameplay] = (std::make_shared<GameplayLevel>());
    _levels[(int)State::Clear] = (std::make_shared<ClearLevel>());
    _levels[(int)State::GameOver] = (std::make_shared<GameOverLevel>());
    _levels[(int)State::Development] = (std::make_shared<DevelopmentLevel>());

    SetSubLevel(_levels[(int)_state]);
}

void Game::StartNewGame()
{
    _hasSword = false;

    _levels[(int)State::Overworld] = std::make_shared<OverworldLevel>();
    _levels[(int)State::SwordCave] = std::make_shared<CaveLevel>();
    _levels[(int)State::Dungeon1] = std::make_shared<DungeonLevel>();

    ChangeLevel(State::Overworld);
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
