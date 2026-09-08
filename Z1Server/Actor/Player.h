#pragma once
#include <Actor/Actor.h>
#include <optional>

class Player final : public Actor
{
public:
    Player(Vector2Int spawnPosition = Vector2Int(1190, 395));

    inline bool IsDead() const noexcept { return info.hp <= 0; }
    std::int32_t TakeDamage(std::int32_t amount);

    bool PerformAttackRequest() noexcept { bool retval = _attackRequested; _attackRequested = false; return retval; }

    inline bool HasInputted() const noexcept { return _lastInputSequence.has_value(); }
    inline std::uint32_t LastInputSequence() const noexcept { return _lastInputSequence.value(); }
    void UpdateInput(const Z1::Protocol::InputCommand& input) noexcept;

    Z1::Protocol::MoveDirection GetInputDirection() const noexcept { return _latestInput.moveDirection; }

private:
    Z1::Protocol::InputCommand _latestInput{};
    std::optional<std::uint32_t> _lastInputSequence;
    bool _attackRequested = false;   // 다음 입력이 올 때까지 flag가 유지되어, '1회'만 발동하도록 플래그
};

