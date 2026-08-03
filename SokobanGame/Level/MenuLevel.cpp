#include "MenuLevel.h"
#include <Game/Game.h>
#include <Core/Input.h>
#include <Render/Renderer.h>

MenuLevel::MenuLevel()
{
    // 메뉴 아이템 생성
    _items.emplace_back(std::make_unique<MenuItem>(
        "Resume Game", []()
        {
            Game& game = dynamic_cast<Game&>(Craft::Engine::Get());
            game.ToggleMenu();
        })
    );

    _items.emplace_back(std::make_unique<MenuItem>(
        "Quit Game", []()
        {
            Craft::Engine::Get().Quit();
        })
    );
}

void MenuLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    // 위/아래 방향키, 엔터, ESC
    const int length = (int)_items.size();
    if (Craft::Input::Get().GetKeyDown(VK_UP))
    {
        _selectedIndex = (_selectedIndex - 1 + length) % length;
    }
    if (Craft::Input::Get().GetKeyDown(VK_DOWN))
    {
        _selectedIndex = (_selectedIndex + 1) % length;
    }

    if (Craft::Input::Get().GetKeyDown(VK_RETURN))
    {
        _items[_selectedIndex]->onSelect();
    }

    if (Craft::Input::Get().GetKeyDown(VK_ESCAPE))
    {
        Game& game = dynamic_cast<Game&>(Craft::Engine::Get());
        game.ToggleMenu();

        // 명시적 초기화
        _selectedIndex = 0;
    }
}

void MenuLevel::Draw()
{
    //Level::Draw();  // MainLevel에는 Actor가 존재하지 않아 호출 안해도 무방

    Craft::Renderer::Get().Submit("Sokoban Game", Craft::Vector2::Zero);

    const int count = (int)_items.size();
    for (int i = 0; i < count; ++i)
    {
        Craft::Color color = (i == _selectedIndex) ? _selectedColor : _unselectedColor;

        Craft::Renderer::Get().Submit(_items[i]->text, Craft::Vector2(0, 2 + i), color);
    }
}
