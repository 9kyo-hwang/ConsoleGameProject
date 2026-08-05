#include "pch.h"
#include "GameLevel.h"
#include <Actor/Player.h>
#include <Actor/EnemySpawner.h>
#include <Actor/GameManager.h>
#include <Engine/Engine.h>
#include <Render/Renderer.h>
#include <sstream>

using namespace Craft;

void GameLevel::OnInitialized()
{
    Level::OnInitialized();

    SpawnActor<Player>();
    SpawnActor<EnemySpawner>();
    
    if (_gameManager = SpawnActor<GameManager>())
    {
        _gameManager->Register_OnPlayerDead([this]() {OnPlayerDead();});
    }
}

void GameLevel::Tick(float deltaTime)
{
    // 게임오버 시 Tick 중지
    if (_state == GameState::GameOver)
    {
        _timer.Tick(deltaTime);
        if (_timer.IsTimeout())
        {
            Engine::Get().Quit();
        }

        return;
    }

    // 게임오버가 아닌 경우에만 액터들의 Tick을 호출해 이동 등 동작하도록.
    Level::Tick(deltaTime);
}

void GameLevel::Draw()
{
    Level::Draw();

    ShowScore();    // 점수는 상시 표시
    if (_state == GameState::GameOver)
    {
        // 플레이어 사망 메시지 표시
        Renderer::Get().Submit("!DEAD!", _gameManager->GetPlayerDeadPosition());
    }
}

void GameLevel::ShowScore()
{
    std::stringstream stream;
    stream << "Score: " << _gameManager->GetScore();

    // 창 좌하단에 표시
    Renderer::Get().Submit(stream.str(), Vector2(0, Engine::Get().GetHeight() - 1));
}

void GameLevel::OnPlayerDead()
{
    // 플레이어 사망 시 상태 변경 & 타이머 설정
    _state = GameState::GameOver;
    _timer.SetTargetTime(GameOverWaitTime);
}
