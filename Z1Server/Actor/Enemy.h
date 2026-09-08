#pragma once
#include <Actor/Actor.h>
#include <Types.h>
#include <Z1Shared/Protocol.h>

// 기존 ServerEnemyState와 TickEnemy()의 책임을 응집시킨 클래스
// id, 종류, 속한 room, 위치 정보, 바라보는 방향, 체력, 사망 유무 소유

class Enemy : public Actor
{
public:
    inline static constexpr std::uint16_t BoxWidth = 8;
    inline static constexpr std::uint16_t BoxHeight = 5;

    Enemy(Z1::Protocol::ActorKind kind, ServerRoomCoordinate home, Vector2Int spawnPosition);

    void MoveTo(Vector2Int position, Z1::Protocol::MoveDirection facing);
    std::int32_t TakeDamage(std::int32_t damage, std::uint32_t serverTick);

    Z1::Protocol::ActorKind GetKind() const noexcept { return info.kind; }
    ServerRoomCoordinate GetHomeRoom() const noexcept { return _home; }
    Vector2Int GetSpawnPosition() const noexcept { return _spawnPosition; }
    bool IsDead() const noexcept { return info.hp <= 0; }

public:
    Vector2Int GetProjectileSpawnPosition();    // 현재는 몹의 중앙에서 Spawn, 추후 가장자리로 옮기는 로직 추가
    bool CanRespawn(std::uint32_t serverTick) const noexcept { return _deadTick + RespawnCooldownTicks <= serverTick; }
    void Respawn();

    /*
    * Moblin
    */
    void TickAttackCooldown() noexcept { if (_attackCooldownTicks > 0) --_attackCooldownTicks; }
    bool IsAttackReady() const noexcept { return _attackCooldownTicks == 0; }
    void ResetAttackCooldown(std::uint32_t ticks = AttackCooldownTicks) noexcept { _attackCooldownTicks = ticks; }

private:
    ServerRoomCoordinate _home;
    Vector2Int _spawnPosition;

    /*
    * Moblin 대상으로 사용되는 변수들
    */

    inline static constexpr std::uint32_t AttackCooldownTicks = 44;   // 44 / 20 = 2.2초 간격
    std::uint32_t _attackCooldownTicks = 0;

    inline static constexpr std::uint32_t RespawnCooldownTicks = 140;   // 140 / 20 = 7초
    std::uint32_t _deadTick = 0;
};
