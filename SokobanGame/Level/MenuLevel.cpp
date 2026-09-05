#include <pch.h>
#include "MenuLevel.h"
#include <Game/Game.h>
#include <Core/Input.h>
#include <Render/Renderer.h>

using namespace Craft;

MenuLevel::MenuLevel()
{
    // 메뉴 아이템 생성
    _items.emplace_back(std::make_unique<MenuItem>(
        "Resume Game", []()
        {
            Game& game = dynamic_cast<Game&>(Engine::Get());
            game.ToggleMenu();
        })
    );

    _items.emplace_back(std::make_unique<MenuItem>(
        "Quit Game", []()
        {
            Engine::Get().Quit();
        })
    );
}

void MenuLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    // 위/아래 방향키, 엔터, ESC
    const int length = (int)_items.size();
    if (Input::Get().GetKeyDown(VK_UP))
    {
        _selectedIndex = (_selectedIndex - 1 + length) % length;
    }
    if (Input::Get().GetKeyDown(VK_DOWN))
    {
        _selectedIndex = (_selectedIndex + 1) % length;
    }

    if (Input::Get().GetKeyDown(VK_RETURN))
    {
        _items[_selectedIndex]->onSelect();
    }

    if (Input::Get().GetKeyDown(VK_ESCAPE))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ToggleMenu();

        // 명시적 초기화
        _selectedIndex = 0;
    }
}

void MenuLevel::Draw()
{
    //Level::Draw();  // MainLevel에는 Actor가 존재하지 않아 호출 안해도 무방

    Renderer::Get().Submit(Sprite::Create("Sokoban Game"), Vector2::Zero);

    const int count = (int)_items.size();
    for (int i = 0; i < count; ++i)
    {
        Color color = (i == _selectedIndex) ? _selectedColor : _unselectedColor;
        Renderer::Get().Submit(Sprite::Create(_items[i]->text, color), Vector2(0, 2 + i));
    }
}
