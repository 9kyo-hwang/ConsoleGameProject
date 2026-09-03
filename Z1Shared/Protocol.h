#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace Z1::Protocol
{
    enum class PacketType : std::uint16_t
    {
        C2S_Enter = 1,              // [size:uint16][type:uint16][version:uint16]
        C2S_Input = 2,
        S2C_Enter = 101,            // [size:uint16][type:uint16][version:uint16][id:uint32]
        S2C_WorldSnapshot = 102,
        S2C_Disconnect = 103,
        S2C_CombatEvent = 104,       // 최신 1개 상태를 유지하는 Snapshot과 달리, 단발성으로 발생
        S2C_EnemyPathDebug = 105    // [tick:u32][id:u32][roomX:32][roomY:32][count:u8][indices:u8 x count]
    };

    inline constexpr std::uint16_t ProtocolVersion = 4;
    inline constexpr std::size_t PacketHeaderSize = sizeof(std::uint16_t) + sizeof(std::uint16_t);
    inline constexpr std::uint16_t MaxPacketSize = 4096;

    struct PacketHeader
    {
        std::uint16_t size = 0; // header를 포함한 전체 패킷 크기
        std::uint16_t type = 0; // raw type. PacketType 유효성 검사는 서버/클라 핸들러 또는 codec 함수에서.
    };

    // 헤더 구조: [size(uint16): 헤더 포함 전체 크기][type(uint16): 패킷 종류][payload..]

    /**************
    * C2S_Input
    ***************/
#pragma region C2S_Input

    // 서버에서 상하좌우 이동 방향을 나타낼 때 사용
    enum class MoveDirection : std::uint8_t
    {
        None = 0,
        Up = 1,
        Down = 2,
        Left = 3,
        Right = 4
    };

    inline constexpr std::uint8_t InputActionAttack = 1 << 0;
    inline constexpr std::uint8_t ValidInputActions = InputActionAttack;

    struct InputCommand
    {
        std::uint32_t sequence;
        MoveDirection moveDirection = MoveDirection::None;
        std::uint8_t actionFlags = 0;   // 공격 유무. 눌린 순간에만 1이 되며 이전 tick의 bit와 비교할 것
    };

    /*
    * C2S_Input: 헤더(크기, 타입) 4바이트 + 페이로드 6바이트(sequence(uint32) + movedir(uint8) + actionflag(uint8, 공격 입력))
    * 예: 00 0A 00 02 00 00 00 01 01 00 -> C2S_Input(00 02), sequence = 1(00 00 00 01), Up(01), action flag 0(00) 
    */
#pragma endregion

    /*************
    * S2C_Snapshot
    **************/
#pragma region S2C_WorldSnapshot
    inline constexpr std::uint8_t PlayerStateDead = 1 << 0;
    inline constexpr std::uint8_t PlayerStateAttacking = 1 << 1;
    inline constexpr std::uint8_t ValidPlayerState = (PlayerStateDead | PlayerStateAttacking);

    inline constexpr std::uint8_t EnemyStateAttacking = 1 << 0;
    inline constexpr std::uint8_t ValidEnemyState = EnemyStateAttacking;

    struct SnapshotPlayerState
    {
        std::uint32_t playerId = 0;
        std::int32_t x = 0;
        std::int32_t y = 0;
        MoveDirection facing = MoveDirection::Up;
        std::int32_t hp = 0;
        std::uint8_t flags = 0;
    };

    enum class EnemyKind : std::uint8_t
    {
        Octorok = 0,
        Moblin = 1,
        Tektite = 2
    };

    struct SnapshotEnemyState
    {
        std::uint32_t id = 0;
        EnemyKind kind = EnemyKind::Octorok;
        std::int32_t x = 0;
        std::int32_t y = 0;
        MoveDirection facing = MoveDirection::Up;
        std::int32_t hp = 0;
        std::uint8_t flags = 0;
    };

    enum class ProjectileKind : std::uint8_t
    {
        Spear = 0   // Moblin의 투사체 무기
    };

    // owner enemy id, 데미지, 수명 등은 x
    struct SnapshotProjectileState
    {
        std::uint32_t id = 0;
        ProjectileKind kind = ProjectileKind::Spear;
        std::int32_t x = 0;
        std::int32_t y = 0;
        MoveDirection direction = MoveDirection::Up;
    };

    // homeRoom, 최초 스폰 위치, AI 타이머는 서버 내부 상태.
    struct WorldSnapshot
    {
        std::uint32_t serverTick = 0;   // 확인용
        std::vector<SnapshotPlayerState> players;
        std::vector<SnapshotEnemyState> enemies;
        std::vector<SnapshotProjectileState> projectiles;
    };
#pragma endregion

    /******************
    * S2C_CombatEvent
    *******************/
#pragma region S2C_CombatEvent
    enum class CombatEventType : std::uint8_t 
    {
        Invalid = 0,
        PlayerSwordAttack,
    };

    struct CombatEvent
    {
        // 클라는 이벤트를 받아 소비하면 끝이라 id는 불필요
        CombatEventType type;
        std::uint32_t actorId = 0;  // 넉백이 포함되려면 targetId도 필요해짐
        MoveDirection direction = MoveDirection::None;  // 검이나 넉백 방향 등
    };
#pragma endregion

#pragma region S2C_EnemyPathDebug
    struct EnemyPathDebug
    {
        std::uint32_t tick = 0;
        std::uint32_t id = 0;
        std::int32_t roomX;
        std::int32_t roomY;

        // Room 하나 당 16 x 11 크기로 최대 176 -> 1바이트
        // 비어있으면 경로 렌더 X
        std::vector<std::uint8_t> tileIndices;  
    };
#pragma endregion
}
