#pragma once
#include <Types.h>
#include <Z1Shared/Protocol.h>

// 기존 ServerEnemyState와 TickEnemy()의 책임을 응집시킨 클래스
// id, 종류, 속한 room, 위치 정보, 바라보는 방향, 체력, 사망 유무 소유

class Enemy
{
public:
    Enemy(std::uint32_t id, Z1::Protocol::EnemyKind kind, ServerRoomCoordinate home, Vector2Int position);
    Z1::Protocol::SnapshotEnemyState BuildSnapshot() const;

    void MoveTo(Vector2Int position, Z1::Protocol::MoveDirection facing);

    inline std::uint32_t GetId() const noexcept { return _id; }
    Z1::Protocol::EnemyKind GetKind() const noexcept { return _kind; }
    ServerRoomCoordinate GetHomeRoom() const noexcept { return _home; }
    Vector2Int GetPosition() const noexcept { return _position; }
    bool IsDead() const noexcept { return _dead; }

private:
    std::uint32_t _id;
    Z1::Protocol::EnemyKind _kind;
    ServerRoomCoordinate _home;

    Vector2Int _spawnPosition;
    Vector2Int _position;

    Z1::Protocol::MoveDirection _facing = Z1::Protocol::MoveDirection::Up;
    std::int32_t _hp = 1;
    bool _dead = false;
};
