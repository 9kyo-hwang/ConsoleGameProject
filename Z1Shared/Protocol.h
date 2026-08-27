#pragma once
#include <cstddef>
#include <cstdint>

namespace Z1::Protocol
{
    enum class PacketType : std::uint16_t
    {
        C2S_Enter = 1,
        C2S_Input = 2,
        S2C_Enter = 101,
        S2C_WorldSnapshot = 102,
        S2C_Disconnect = 103
    };

    inline constexpr std::uint16_t ProtocolVersion = 1;
    inline constexpr std::size_t PacketHeaderSize = 4;
    inline constexpr std::uint16_t MaxPacketSize = 4096;

    // 헤더 구조: [size(uint16): 헤더 포함 전체 크기][type(uint16): 패킷 종류][payload..]
}
