#include "pch.h"
#include "NetworkProjectile.h"
#include <Component/SpriteRendererComponent.h>

using namespace Craft;
using namespace Z1::Protocol;

NetworkProjectile::NetworkProjectile(Vector2 position, std::uint32_t id, ProjectileKind kind, MoveDirection direction)
    : Super(position), _id(id), _kind(kind), _direction(direction)
{
    bool horizontal = direction == MoveDirection::Left || direction == MoveDirection::Right;

    std::string image = horizontal ? "-" : "|";

    _renderer = AddComponent<SpriteRendererComponent>(Sprite::Create(image, Color::DarkYellow), 12);
}

void NetworkProjectile::ApplySnapshot(const Z1::Protocol::SnapshotProjectileState& state)
{
    assert(_id == state.id);
    assert(_kind == state.kind);
    assert(_direction == state.direction);

    if (_id != state.id ||
        _kind != state.kind ||
        _direction != state.direction)
    {
        return;
    }

    // Tick 기반 이동, 충돌 판정, 피해량, 수명 timer, owner 등은 서버에서 결정
    SetPosition(Vector2(state.x, state.y));
}
