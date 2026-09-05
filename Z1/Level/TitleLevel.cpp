#include "pch.h"
#include "TitleLevel.h"
#include <Render/Renderer.h>
#include <Core/Input.h>
#include <Game/Game.h>
#include <fstream>

using namespace Craft;

TitleLevel::TitleLevel()
    : _connectTimer(3.f)    // 서버 접속 대기 3초
    , _noticeTimer(3.f)
{

}

void TitleLevel::OnInitialized()
{
    // 1. BGM Play
    // 2. Title Sprites Create

    Level::OnInitialized();
    _titleSprite = LoadTitleSprite("../Content/Z1/Title.txt");
}

void TitleLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_bgmStarted)
    {
        Engine::Get().PlayBGM("Z1/01. Title Screen.wav");
        _bgmStarted = true;
    }
}

void TitleLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (!_noticeMessage.empty())
    {
        _noticeTimer.Tick(deltaTime);
        if (_noticeTimer.TimeOver())
        {
            _noticeMessage.clear();
            _noticeTimer.Reset();
        }
    }

    Game& game = dynamic_cast<Game&>(Engine::Get());
    if (_state == State::Connecting)
    {
        // true: success or pending -> success면 state 바껴서 진입 안하고, pending이면 안바껴서 다시 진입
        // false: fail -> state 바뀌고 return 안돼서 아래 로직 수행
        if (OnConnectingServer(game, deltaTime))
        {
            return;
        }
    }

    constexpr int len = (int)GameMode::END;
    if (Input::Get().GetKeyDown(VK_UP))
    {
        _gameMode = (_gameMode - 1 + len) % len;
    }
    if (Input::Get().GetKeyDown(VK_DOWN))
    {
        _gameMode = (_gameMode+ 1) % len;
    }

    if (Input::Get().GetKeyDown(VK_RETURN))
    {
        switch ((GameMode)_gameMode)
        {
        case GameMode::Localplay: OnSelectLocalplayMode(game); break;
        case GameMode::Multiplay: OnSelectMultiplayMode(game); break;
        }
    }
}

void TitleLevel::Draw()
{
    //Level::Draw();  // actor가 없어 굳이 호출할 필요 없음

    if (!_titleSprite) return;

    const Vector2 titlePosition(0, 3);
    Renderer::Get().Submit(_titleSprite, titlePosition);

    const Vector2 TitleSpriteSize = _titleSprite->GetSize();
    const std::string localplay = "LocalPlay";
    const std::string multiplay = "MultiPlay";

    for (int i = 0; i < (int)GameMode::END; ++i)
    {
        const Color color = (i == _gameMode) ? _selectedColor : _unselectedColor;
        const Vector2 offset(
            (TitleSpriteSize.x - 9) / 2, // (sprite 폭 - 문자열 길이) 절반
            (_titleSprite->GetSize().y + 2 + i)
        );

        Renderer::Get().Submit(
            Sprite::Create(
                i == (int)GameMode::Localplay ? localplay : multiplay, 
                color
            ), titlePosition + offset
        );
    }

    if (_state == State::Menu && !_noticeMessage.empty())
    {
        const Vector2 offset(
            (TitleSpriteSize.x - _noticeMessage.size()) / 2,
            TitleSpriteSize.y + 5
        );

        Renderer::Get().Submit(Sprite::Create(_noticeMessage, Color::Red), titlePosition + offset);
    }
    else if (_state == State::Connecting)
    {
        const std::string noticeMessage = "Connecting to server...";
        const Vector2 offset(
            (TitleSpriteSize.x - noticeMessage.size()) / 2,
            TitleSpriteSize.y + 5
        );

        Renderer::Get().Submit(Sprite::Create(noticeMessage, Color::Red), titlePosition + offset);
    }
}

void TitleLevel::EndPlay()
{
    Level::EndPlay();

    Engine::Get().StopBGM();
    _bgmStarted = false;

    // 레벨이 달라졌을 때만 호출하기 때문에, 초기화를 여기서 수행하도록 변경
    _state = State::Menu;
    _gameMode = 0;
}

std::shared_ptr<const Sprite> TitleLevel::LoadTitleSprite(const FilePath& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        return nullptr;
    }

    std::vector<std::string> lines;
    std::string line;

    int width = 0;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        width = std::max(width, (int)line.size());
        lines.emplace_back(std::move(line));
    }

    if (lines.empty() || width == 0)
    {
        return nullptr;
    }

    const int height = (int)lines.size();

    std::vector<SpriteCell> cells;
    cells.reserve((size_t)width * height);

    for (const std::string& line : lines)
    {
        for (char glyph : line)
        {
            cells.emplace_back(glyph, (WORD)Color::White, glyph == ' ');
        }
    }

    return std::make_shared<Sprite>(Vector2(width, height), std::move(cells));
}

void TitleLevel::OnSelectLocalplayMode(Game& game)
{
    game.StartNewGame(GameMode::Localplay);
}

void TitleLevel::OnSelectMultiplayMode(Game& game)
{
    if (!game.ConnectToServer())
    {
        SetNoticeMessage("서버에 연결할 수 없습니다.");
        _state = State::Menu;
        return;
    }

    _state = State::Connecting;
    _connectTimer.Set(3.f);
}

bool TitleLevel::OnConnectingServer(Game& game, float deltaTime)
{
    // 서버 Enter 처리를 위해.
    game.PumpNetwork();

    if (game.GetLocalPlayerId().has_value())
    {
        // 발급 완료 시 시작.
        _state = State::Menu;
        _connectTimer.Reset();
        _noticeMessage.clear();
        game.StartNewGame(GameMode::Multiplay);
        return true;
    }

    _connectTimer.Tick(deltaTime);
    if (!game.IsServerConnected() || _connectTimer.TimeOver())
    {
        game.Disconnect();
        _state = State::Menu;
        SetNoticeMessage("서버 응답이 없습니다.");
        return false;
    }

    return true;    // pending
}
