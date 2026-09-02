#pragma once
#include <Actor/Actor.h>
#include <Z1Shared/Protocol.h>

namespace Craft
{
    class Sprite;
    class SpriteRendererComponent;
}

/// <summary>
/// 서버로부터 받은 데이터를 소유하는 표시 전용 객체
/// 표현하는 것: id, 방향 정보, 체력, 공격 및 사망 유무, Sprite
/// </summary>
class NetworkPlayer : public Craft::Actor
{
    TYPE_DECLARATIONS(NetworkPlayer, Craft::Actor)

public:
    NetworkPlayer(Craft::Vector2 position, std::uint32_t playerId);
    
    void ApplySnapshot(const Z1::Protocol::SnapshotPlayerState& state);

    inline std::uint32_t GetPlayerId() const noexcept { return _playerId; }
    inline Z1::Protocol::MoveDirection GetFacing() const noexcept { return _facing; }
    std::int32_t GetHp() const noexcept { return _hp; }
    inline bool IsDead() const noexcept { return (_flags & Z1::Protocol::PlayerStateDead) != 0; }
    inline bool IsAttacking() const noexcept { return (_flags & Z1::Protocol::PlayerStateAttacking) != 0; }

private:
    std::shared_ptr<const Craft::Sprite> CreateSprite();

private:
    std::shared_ptr<Craft::SpriteRendererComponent> _renderer;

    bool _hasSnapshot = false;  // 최초 1회 Snapshot을 받은 뒤론 true
    std::uint32_t _playerId = 0;    // Snapshot마다 바뀌지 않음(생성할 때 ID와 Snapshot이 일치해야 함)
    Z1::Protocol::MoveDirection _facing = Z1::Protocol::MoveDirection::Up;
    std::int32_t _hp = 0;
    std::uint8_t _flags = 0;
};

