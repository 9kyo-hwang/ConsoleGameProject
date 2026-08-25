#include <pch.h>
#include "Wall.h"
#include <Component/SpriteRendererComponent.h>
#include <Component/BoxComponent.h>

using namespace Craft;

// 벽은 충돌도, 렌더링도 필요
Wall::Wall(const Craft::Vector2& position)
    : Super(position)
{
    // 사실 벽은 겹칠 일이 없어 소팅 오더는 의미 없음
    AddComponent<SpriteRendererComponent>(Sprite::Create("#", Color::White), 2);
    AddComponent<BoxComponent>(1);  // width = 1
}
