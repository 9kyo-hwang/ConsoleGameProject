#include "pch.h"
#include "NetworkProjectile.h"
#include <Component/SpriteRendererComponent.h>

using namespace Craft;
using namespace Z1::Protocol;

NetworkProjectile::NetworkProjectile(Vector2 position, std::uint32_t id, ActorKind kind, MoveDirection direction)
    : Super(position), _id(id), _kind(kind), _direction(direction)
{
    bool horizontal = direction == MoveDirection::Left || direction == MoveDirection::Right;

    std::string image = horizontal ? "-" : "|";

    _renderer = AddComponent<SpriteRendererComponent>(Sprite::Create(image, Color::DarkYellow), 12);
}

void NetworkProjectile::ApplySnapshot(const Z1::Protocol::ActorInfo& info)
{
    assert(_id == info.id);
    assert(_kind == info.kind);
    assert(_direction == info.direction);

    if (_id != info.id ||
        _kind != info.kind ||
        _direction != info.direction)
    {
        return;
    }

    // Tick 기반 이동, 충돌 판정, 피해량, 수명 timer, owner 등은 서버에서 결정
    SetPosition(Vector2(info.x, info.y));
}
