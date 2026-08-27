#pragma once

#include <WinSock2.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>
#include <vector>
#include <bit>

#include <Z1Shared/Protocol.h>

// 클라이언트, 서버가 동시에 사용하는 유틸리티 함수들
// Byte 읽기/쓰기, 패킷 생성기 등
namespace Z1::Protocol
{
    using Byte = std::uint8_t;
    
    // 방향, flags 값은 1바이트라 변환 필요없으나, API 형태를 맞추기 위해 헬퍼 추가
    inline void WriteU8(std::vector<Byte>& output, std::uint8_t value)
    {
        output.push_back(value);
    }

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

    inline void Write32(std::vector<Byte>& output, std::int32_t value)
    {
        // bit_cast<T>: 값-대-값으로 비트 패턴을 T 타입으로 재해석
        // Trivially copyable(단순 복사 가능) 제약 조건 존재
        // - memcpy 같은 메모리 블럭 단위 복사를 해도 안전한 것들
        // - 즉 별도의 복사 생성자/소멸자/가상함수 등이 없고 바이트 단위 복사만으로 동일한 의미를 가짐
        // 소스와 타겟 타입의 크기가 같아야 함
        WriteU32(output, std::bit_cast<std::uint32_t>(value));
    }

    inline bool ReadU8(std::span<const Byte> input, std::size_t& offset, std::uint8_t& output)
    {
        if (offset >= input.size())
        {
            return false;
        }

        output = input[offset++];
        return true;
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

    inline bool Read32(std::span<const Byte> input, std::size_t& offset, std::int32_t& output)
    {
        std::uint32_t raw = 0;
        if (!ReadU32(input, offset, raw)) 
        {
            return false;
        }

        output = std::bit_cast<std::int32_t>(raw);
        return true;
    }

    inline bool BuildPacket(PacketType type, std::span<const Byte> payload, std::vector<Byte>& outPacket)
    {
        // 패킷 내용물이 너무 크면 안됨
        if (payload.size() > MaxPacketSize - PacketHeaderSize)
        {
            return false;
        }

        std::uint16_t packetSize = (std::uint16_t)(PacketHeaderSize + payload.size());

        outPacket.clear();
        outPacket.reserve(packetSize);

        WriteU16(outPacket, packetSize);    // 패킷 크기 적고
        WriteU16(outPacket, (std::uint16_t)type);   // 패킷 종류 넣고
        outPacket.insert(outPacket.end(), payload.begin(), payload.end());  // 헤더 뒷부분부터 payload로 채워넣기
        
        return true;
    }
}