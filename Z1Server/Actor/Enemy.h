#pragma once
#include <OverworldSimulation.h>

struct Vector2Int
{
    int x = 0;
    int y = 0;

    auto operator<=>(const Vector2Int&) const = default;
};

// 이동하기를 희망하는 방향
struct EnemyMovementIntent
{
    Z1::Protocol::MoveDirection direction = Z1::Protocol::MoveDirection::None;
};

// 기존 ServerEnemyState와 TickEnemy()의 책임을 응집시킨 클래스
// id, 종류, 속한 room, 위치 정보, 바라보는 방향, 체력, 사망 유무 소유
// 행동 상태와 경로 캐싱 및 Snapshot 상태 생성
// Tick()을 통해 "해당 방향으로 이동"을 시도하는 intent 제출
class Enemy
{
public:
    Enemy(std::uint32_t id, Z1::Protocol::EnemyKind kind, ServerRoomCoordinate home, Vector2Int position);

    inline std::uint32_t GetId() const noexcept { return _id; }
    ServerRoomCoordinate GetHomeRoom() const noexcept { return _home; }
    Vector2Int GetPosition() const noexcept { return _position; }
    bool IsDead() const noexcept { return _dead; }

    SnapshotEnemyState BuildSnapshot() const;

private:
    std::uint32_t _id;
    Z1::Protocol::EnemyKind _kind;
    ServerRoomCoordinate _home;
    Vector2Int _spawnPosition;
    Vector2Int _position;
    Z1::Protocol::MoveDirection _facing;
    std::int32_t _hp = 1;
    bool _dead = false;
};
