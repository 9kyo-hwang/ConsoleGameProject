#include "pch.h"
#include "TitleLevel.h"
#include <Render/Renderer.h>
#include <Core/Input.h>
#include <Game/Game.h>
#include <fstream>

using namespace Craft;

TitleLevel::TitleLevel()
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

    if (Input::Get().GetKeyDown('1'))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Title);
    }

    if (Input::Get().GetKeyDown('2'))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Overworld);
    }

    if (Input::Get().GetKeyDown('3'))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::Clear);
    }
}

void TitleLevel::Draw()
{
    Level::Draw();

    if (!_titleSprite) return;

    Renderer::Get().Submit(_titleSprite, Vector2(0, 3));
}

void TitleLevel::EndPlay()
{
    Level::EndPlay();

    Engine::Get().StopBGM();
    _bgmStarted = false;
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
