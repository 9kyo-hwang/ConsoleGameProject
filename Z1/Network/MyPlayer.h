#pragma once
#include <Network/NetworkPlayer.h>
#include <Z1Shared/Protocol.h>

// Snapshot 적용 + [입력]을 받아 서버에게 의도 전달
class MyPlayer : public NetworkPlayer
{
    TYPE_DECLARATIONS(MyPlayer, NetworkPlayer)

public:
    MyPlayer(Craft::Vector2 position, std::uint32_t playerId);

    void Tick(float deltaTime) override;

private:
    Z1::Protocol::MoveDirection InputToMoveDir() const;

private:
    Z1::Protocol::MoveDirection _lastSentMoveDir = Z1::Protocol::MoveDirection::None;
};

