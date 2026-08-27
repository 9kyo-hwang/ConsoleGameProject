#pragma once

#include <WinSock2.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>

// TODO: 16bit 값 처리자. WriteU16, ReadU16...
namespace Z1::Protocol
{
    using Byte = std::uint8_t;
    
    inline void WriteU16(std::vector<Byte>& output, std::uint16_t value)
    {
        const u_short netshort = ::htons((u_short)value);    // network big-endian

        const std::size_t offset = output.size();
        output.resize(offset + sizeof(netshort));

        std::memcpy(output.data() + offset, &netshort, sizeof(netshort));
    }

    inline void WriteU32(std::vector<Byte>& output, std::uint32_t value)
    {
        const u_long netlong = ::htonl((u_long)value);

        const std::size_t offset = output.size();
        output.resize(offset + sizeof(netlong));

        std::memcpy(output.data() + offset, &netlong, sizeof(netlong));
    }

    inline bool ReadU16(std::span<const Byte> input, std::size_t& offset, std::uint16_t& output)
    {
        // 읽으려는 위치가 전체 input을 넘어갔거나, 읽으려는 크기가 uint16 크기가 안되는 케이스 거르기
        if (offset > input.size() || input.size() - offset < sizeof(std::uint16_t))
        {
            return false;
        }

        std::uint16_t netshort = 0;
        std::memcpy(&netshort, input.data() + offset, sizeof(netshort));

        offset += sizeof(netshort);
        output = ::ntohs(netshort);    // pc little-endian

        return true;
    }

    inline bool ReadU32(std::span<const Byte> input, std::size_t& offset, std::uint32_t& output)
    {
        // 읽으려는 위치가 전체 input을 넘어갔거나, 읽으려는 크기가 uint16 크기가 안되는 케이스 거르기
        if (offset > input.size() || input.size() - offset < sizeof(std::uint32_t))
        {
            return false;
        }

        std::uint32_t netlong = 0;
        std::memcpy(&netlong, input.data() + offset, sizeof(netlong));

        offset += sizeof(netlong);
        output = ::ntohl(netlong);    // pc little-endian

        return true;
    }
}