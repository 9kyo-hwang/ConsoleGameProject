#pragma once
#include <Level/Level.h>
#include <string>
#include <Interface/ICanPlayerMove.h>

class GameLevel : public Craft::Level, public ICanPlayerMove
{
public:
    void OnInitialized() override;
    void Draw() override;

    // ICanPlayerMove을(를) 통해 상속됨
    bool CanMoveTo(const Craft::Vector2& from, const Craft::Vector2& to) override;

private:
    void LoadMap(const std::string& filename);
    bool IsGameCleared();

private:
    // 소코반: 박스를 목표 지점에 가져다놓는 게임 -> 개수 기록
    int _targetScore = 0;
    bool _isGameCleared = false;
};

