#pragma once
#include <Level/Level.h>
#include <Util/Timer.h>

class GameManager;
class GameLevel : public Craft::Level
{
    enum class GameState
    {
        Playing,
        GameOver,
    };

public:
    GameLevel() = default;
    ~GameLevel() override = default;

private:
    void OnInitialized() override;
    void Tick(float deltaTime) override;    // 게임오버 상태 지속시간 측정
    void Draw() override;   // 게임오버 UI 표시

    void ShowScore();
    void OnPlayerDead();    // GM쪽에서 호출할 callback

private:
    std::shared_ptr<GameManager> _gameManager;
    GameState _state = GameState::Playing;
    Timer _timer;   // 게임 오버 시 대기 시간 측정용
    const float GameOverWaitTime = 1.f; // 게임 오버 시 2초간 대기
};

