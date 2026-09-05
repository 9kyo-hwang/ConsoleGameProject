#pragma once
#include <Level/Level.h>
#include <string>
#include <Math/Color.h>

// 메뉴 아이템 구조체
struct MenuItem
{
    // 메뉴가 선택됐을 때 실행될 함수 타입(함수 포인터)
    using OnSelect = void(*)();

    MenuItem(const std::string& text, OnSelect onSelect)
        : text(text), onSelect(onSelect)
    {

    }

    ~MenuItem() = default;

    // 속성
    std::string text;
    OnSelect onSelect = nullptr;
};

class MenuLevel : public Craft::Level
{
public:
    MenuLevel();
    ~MenuLevel() = default;

private:
    void Tick(float deltaTime) override;  // 입력 처리때문
    void Draw() override;

private:
    int _selectedIndex = 0;  // 현재 활성화된 메뉴 목록
    Craft::Color _selectedColor = Craft::Color::Green; // 활성화된 메뉴 색상
    Craft::Color _unselectedColor = Craft::Color::White;

    std::vector<std::unique_ptr<MenuItem>> _items;
};
