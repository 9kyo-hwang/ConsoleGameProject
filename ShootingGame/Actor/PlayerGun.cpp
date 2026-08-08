#include "pch.h"
#include "PlayerGun.h"
#include <Component/SpriteRendererComponent.h>

using namespace Craft;

PlayerGun::PlayerGun(const Vector2& localPosition)
    : Super(localPosition)
{
    AddComponent<SpriteRendererComponent>("^", Color::Blue, 6);
}

Vector2 PlayerGun::GetFirePosition() const
{
    // 한 칸 위에서 발사
    return GetWorldPosition() + Vector2(0, -1);
}
