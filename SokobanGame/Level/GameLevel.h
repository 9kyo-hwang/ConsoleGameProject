#pragma once
#include <Level/Level.h>
#include <string>

class GameLevel : public Craft::Level
{
public:
    void OnInitialized() override;
    void Draw() override;

private:
    void LoadMap(const std::string& filename);

private:
    // 소코반: 박스를 목표 지점에 가져다놓는 게임 -> 개수 기록
    int _targetScore = 0;
    bool _isCleared = false;
};

