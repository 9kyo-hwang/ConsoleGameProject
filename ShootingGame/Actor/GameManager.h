#pragma once
#include <Actor/Actor.h>
#include <functional>

class GameManager : public Craft::Actor
{
    TYPE_DECLARATIONS(GameManager, Craft::Actor)

public:
    // Player에 사망 시 실행할 메서드가 정의됨.
    // 메서드를 받기 위해 function<>으로 콜백 정의
    using OnPlayerDead = std::function<void()>;

    GameManager() = default;
    ~GameManager() override = default;

    inline void Register_OnPlayerDead(OnPlayerDead onPlayerDead) { _onPlayerDead = onPlayerDead; }

    inline Craft::Vector2 GetPlayerDeadPosition() const { return _playerDeadPosition; }
    void SetPlayerDead(const Craft::Vector2& position);

    inline int GetScore() const { return _score; }
    inline void SetScore(int score) { _score = score; }

    inline bool IsPlayerDead() const { return _isPlayerDead; }

private:
    int _score = 0; // 관리는 GM이, 표시는 Level이
    bool _isPlayerDead = false;
    Craft::Vector2 _playerDeadPosition{};   // 메시지 표시를 위한 위치값
    OnPlayerDead _onPlayerDead = nullptr;
};

