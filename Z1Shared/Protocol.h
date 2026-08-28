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
        S2C_Disconnect = 103
    };

    inline constexpr std::uint16_t ProtocolVersion = 1;
    inline constexpr std::size_t PacketHeaderSize = 4;
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

    /*************
    * S2C_Snapshot
    **************/

    inline constexpr std::uint8_t PlayerStateDead = 1 << 0;
    inline constexpr std::uint8_t PlayerStateAttacking = 1 << 1;

    struct SnapshotPlayerState
    {
        std::uint32_t playerId = 0;
        std::int32_t x = 0;
        std::int32_t y = 0;
        MoveDirection facing = MoveDirection::Up;
        std::int32_t hp = 0;
        std::uint8_t flags = 0;
    };

    struct WorldSnapshot
    {
        std::uint32_t serverTick = 0;   // 확인용
        std::vector<Z1::Protocol::SnapshotPlayerState> players;
    };
}
