#pragma once
#include <Level/Level.h>
#include <memory>
#include <filesystem>
#include <Util/Timer.h>

namespace Craft
{
    class Sprite;
}

class Game;

class TitleLevel : public Craft::Level
{
    using FilePath = std::filesystem::path;

    enum class State
    {
        Menu,       // 메뉴에서 모드 선택 중
        Connecting, // Multiplayer 모드 선택해서 서버에 접속 중
    };

public:
    TitleLevel();
    ~TitleLevel() override = default;

    void OnInitialized() override;
    void BeginPlay() override;
    void Tick(float deltaTime) override;
    void Draw() override;
    void EndPlay() override;

    inline void SetNoticeMessage(const std::string& message, float duration = 3.f)
    {
        _state = State::Menu;   // 알림을 띄울 땐 메뉴 조작 가능한 상태로
        _noticeMessage = message;
        _noticeTimer.Set(duration);
    }

private:
    std::shared_ptr<const Craft::Sprite> LoadTitleSprite(const FilePath& path);
    void OnSelectLocalplayMode(Game& game);
    void OnSelectMultiplayMode(Game& game);
    bool OnConnectingServer(Game& game, float deltaTime);

private:
    std::shared_ptr<const Craft::Sprite> _titleSprite;
    bool _bgmStarted = false;

    int _gameMode = 0;
    Craft::Color _selectedColor = Craft::Color::Green; // 활성화된 메뉴 색상
    Craft::Color _unselectedColor = Craft::Color::White;
    State _state = State::Menu;
    Timer _connectTimer;

    std::string _noticeMessage{};
    Timer _noticeTimer;
};

