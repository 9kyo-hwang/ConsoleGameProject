#pragma once
#include <Actor/Actor.h>
#include <optional>

class Player final : public Actor
{
public:
    Player(Vector2Int spawnPosition = Vector2Int(1190, 395));

    bool IsDead() const noexcept { return info.hp <= 0; }
    std::int32_t TakeDamage(std::int32_t amount);

    Z1::Protocol::InputCommand latestInput{};
    std::optional<std::uint32_t> lastInputSequence;
    bool attackRequested = false;   // 다음 입력이 올 때까지 flag가 유지되어, '1회'만 발동하도록 플래그
};

