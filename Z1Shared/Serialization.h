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

    // span + offset
    class PacketReader
    {
    public:
        explicit PacketReader(std::span<const Byte> bytes) noexcept : _bytes(bytes), _offset(0) {}

        bool IsAtEnd() const noexcept { return _offset == _bytes.size(); }
        std::size_t Remaining() const noexcept { return _bytes.size() - _offset; }

        bool ReadU8(std::uint8_t& output)
        {
            if (_offset >= _bytes.size())
            {
                return false;
            }

            output = _bytes[_offset++];
            return true;
        }

        bool ReadU16(std::uint16_t& output)
        {
            // 읽으려는 위치가 전체 input을 넘어갔거나, 읽으려는 크기가 uint16 크기가 안되는 케이스 거르기
            if (_offset > _bytes.size() || _bytes.size() - _offset < sizeof(std::uint16_t))
            {
                return false;
            }

            std::uint16_t netshort = 0;
            std::memcpy(&netshort, _bytes.data() + _offset, sizeof(netshort));

            _offset += sizeof(netshort);
            output = ::ntohs(netshort);    // pc little-endian

            return true;
        }

        bool ReadU32(std::uint32_t& output)
        {
            // 읽으려는 위치가 전체 input을 넘어갔거나, 읽으려는 크기가 uint16 크기가 안되는 케이스 거르기
            if (_offset > _bytes.size() || _bytes.size() - _offset < sizeof(std::uint32_t))
            {
                return false;
            }

            std::uint32_t netlong = 0;
            std::memcpy(&netlong, _bytes.data() + _offset, sizeof(netlong));

            _offset += sizeof(netlong);
            output = ::ntohl(netlong);    // pc little-endian

            return true;
        }

        bool Read32(std::int32_t& output)
        {
            std::uint32_t raw = 0;
            if (!ReadU32(raw))
            {
                return false;
            }

            output = std::bit_cast<std::int32_t>(raw);
            return true;
        }

    private:
        std::span<const Byte> _bytes{};
        std::size_t _offset = 0;
    };

    class PacketWriter
    {
    public:
        explicit PacketWriter(std::size_t reserveSize = 0)
        {
            _bytes.reserve(reserveSize);
        }

        std::span<const Byte> Bytes() const noexcept { return { _bytes.data(), _bytes.size() }; }
        std::vector<Byte> TakeBytes() const noexcept { return std::move(_bytes); }

        void Append(std::span<const Byte> bytes)
        {
            _bytes.insert(_bytes.end(), bytes.begin(), bytes.end());
        }

        void WriteU8(std::uint8_t value)
        {
            _bytes.push_back(value);
        }

        void WriteU16(std::uint16_t value)
        {
            const u_short netshort = ::htons((u_short)value);    // network big-endian

            const std::size_t offset = _bytes.size();
            _bytes.resize(offset + sizeof(netshort));

            std::memcpy(_bytes.data() + offset, &netshort, sizeof(netshort));
        }

        void WriteU32(std::uint32_t value)
        {
            const u_long netlong = ::htonl((u_long)value);

            const std::size_t offset = _bytes.size();
            _bytes.resize(offset + sizeof(netlong));

            std::memcpy(_bytes.data() + offset, &netlong, sizeof(netlong));
        }

        void Write32(std::int32_t value)
        {
            // bit_cast<T>: 값-대-값으로 비트 패턴을 T 타입으로 재해석
            // Trivially copyable(단순 복사 가능) 제약 조건 존재
            // - memcpy 같은 메모리 블럭 단위 복사를 해도 안전한 것들
            // - 즉 별도의 복사 생성자/소멸자/가상함수 등이 없고 바이트 단위 복사만으로 동일한 의미를 가짐
            // 소스와 타겟 타입의 크기가 같아야 함
            WriteU32(std::bit_cast<std::uint32_t>(value));
        }

    private:
        std::vector<Byte> _bytes{};
    };

    inline bool BuildPacket(PacketType type, std::span<const Byte> payload, std::vector<Byte>& outPacket)
    {
        // 패킷 내용물이 너무 크면 안됨
        if (payload.size() > MaxPacketSize - PacketHeaderSize)
        {
            return false;
        }

        const std::uint16_t packetSize = (std::uint16_t)(PacketHeaderSize + payload.size());

        PacketWriter writer(packetSize);
        writer.WriteU16(packetSize);
        writer.WriteU16((std::uint16_t)type);
        writer.Append(payload);

        outPacket = writer.TakeBytes();
        return true;
    }
}