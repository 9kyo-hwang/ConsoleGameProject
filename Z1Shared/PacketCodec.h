#pragma once
#include <Z1Shared/Serialization.h>
#include <Z1Shared/Protocol.h>
#include <vector>
#include <span>

namespace Z1::Protocol
{
    // Header-Only라 전부 인라인

    // 0 ~ 4 의 값이 아니라면 문제
    inline bool IsValidMoveDirection(std::uint8_t direction) noexcept
    {
        return direction <= (std::uint8_t)MoveDirection::Right;
    }

    inline bool BuildPacket_C2SEnter(std::vector<Byte>& packet)
    {
        PacketWriter payload(sizeof(std::uint16_t));
        payload.WriteU16(ProtocolVersion);

        return BuildPacket(PacketType::C2S_Enter, payload.Bytes(), packet);
    }

    inline bool ParsePayload_C2SEnter(std::span<const Byte> payload, std::uint16_t& version)
    {
        PacketReader reader(payload);

        if (!reader.ReadU16(version) || !reader.IsAtEnd())
        {
            return false;
        }

        return version == ProtocolVersion;
    }

    inline bool BuildPacket_S2CEnter(std::uint32_t playerId, std::vector<Byte>& packet)
    {
        if (playerId == 0) return false;

        PacketWriter payload(sizeof(std::uint16_t) + sizeof(std::uint32_t));
        payload.WriteU16(ProtocolVersion);
        payload.WriteU32(playerId);

        return BuildPacket(PacketType::S2C_Enter, payload.Bytes(), packet);
    }

    inline bool ParsePayload_S2CEnter(std::span<const Byte> payload, std::uint32_t& playerId)
    {
        PacketReader reader(payload);

        std::uint16_t version = 0;
        if (!reader.ReadU16(version) || !reader.ReadU32(playerId) || !reader.IsAtEnd())
        {
            return false;
        }

        return version == ProtocolVersion && playerId != 0;
    }

    inline bool BuildPacket_C2SInput(const InputCommand& input, std::vector<Byte>& packet)
    {
        const std::uint8_t direction = (std::uint8_t)input.moveDirection;
        if (!IsValidMoveDirection(direction) || (input.actionFlags & ~ValidInputActions) != 0)
        {
            return false;
        }

        PacketWriter payload(sizeof(std::uint32_t) + sizeof(std::uint8_t) + sizeof(std::uint8_t));
        payload.WriteU32(input.sequence);
        payload.WriteU8(direction);
        payload.WriteU8(input.actionFlags);

        return BuildPacket(PacketType::C2S_Input, payload.Bytes(), packet);
    }

    inline bool ParsePayload_C2SInput(std::span<const Byte> payload, InputCommand& input)
    {
        PacketReader reader(payload);
        std::uint8_t direction = 0, actionFlags = 0;

        if (!reader.ReadU32(input.sequence) || !reader.ReadU8(direction)
            || !reader.ReadU8(actionFlags) || !reader.IsAtEnd())
        {
            return false;
        }

        // [11111...0]이랑 & 했는데 0이 아니라면 flag로 2 이상의 값이 들어왔다는 뜻
        if (!IsValidMoveDirection(direction) || (actionFlags & ~ValidInputActions) != 0)
        {
            return false;
        }

        input.moveDirection = (MoveDirection)direction;
        input.actionFlags = actionFlags;
        
        return true;
    }

    inline constexpr std::size_t SnapshotPlayerStateSize 
        = sizeof(std::uint32_t) // id 
        + sizeof(std::int32_t)  // x
        + sizeof(std::int32_t)  // y
        + sizeof(std::uint8_t)  // facing
        + sizeof(std::int32_t)  // hp
        + sizeof(std::uint8_t); // flags

    inline constexpr std::size_t WorldSnapshotFixedPayloadSize
        = sizeof(std::uint32_t)     // serverTick
        + sizeof(std::uint16_t)     // playerCount
        + sizeof(std::uint16_t)     // enemyCount
        + sizeof(std::uint16_t);    // projectileCount

    inline constexpr std::size_t MaxSnapshotPlayers
        = (MaxPacketSize - PacketHeaderSize - WorldSnapshotFixedPayloadSize) 
        / SnapshotPlayerStateSize;

    inline bool BuildPacket_S2CWorldSnapshot(const WorldSnapshot& snapshot, std::vector<Byte>& packet)
    {
        if (snapshot.players.size() > MaxSnapshotPlayers)
        {
            return false;
        }

        const std::size_t payloadSize = WorldSnapshotFixedPayloadSize + snapshot.players.size() * SnapshotPlayerStateSize;

        PacketWriter payload(payloadSize);
        payload.WriteU32(snapshot.serverTick);
        payload.WriteU16((std::uint16_t)snapshot.players.size());

        for (const SnapshotPlayerState& player : snapshot.players)
        {
            const std::uint8_t facing = (std::uint8_t)player.facing;
            if (!IsValidMoveDirection(facing) || (player.flags & ~(PlayerStateDead | PlayerStateAttacking)) != 0)
            {
                return false;
            }

            payload.WriteU32(player.playerId);
            payload.Write32(player.x);
            payload.Write32(player.y);
            payload.WriteU8(facing);
            payload.Write32(player.hp);
            payload.WriteU8(player.flags);
        }

        payload.WriteU16(0);    // enemy
        payload.WriteU16(0);    // projectile

        return BuildPacket(PacketType::S2C_WorldSnapshot, payload.Bytes(), packet);
    }

    inline bool ParsePayload_S2CWorldSnapshot(std::span<const Byte> payload, WorldSnapshot& snapshot)
    {
        PacketReader reader(payload);

        // 서버 틱 + 플레이어 수 + [id, x, y, facing, hp, flags] 반복 + 적 수 + 투사체 수
        WorldSnapshot parsed;
        std::uint16_t playerCount = 0;

        if (!reader.ReadU32(parsed.serverTick) || !reader.ReadU16(playerCount))
        {
            return false;
        }

        // Player 배열 뒤에는 Enemy/Projectile count 4바이트가 반드시 남아야 한다.
        constexpr std::size_t TailSize = sizeof(std::uint16_t) + sizeof(std::uint16_t);

        if (reader.Remaining() < TailSize)
        {
            return false;
        }

        const std::size_t playerBytes = reader.Remaining() - TailSize;

        if (playerCount > MaxSnapshotPlayers || playerCount > playerBytes / SnapshotPlayerStateSize)
        {
            return false;
        }

        parsed.players.reserve(playerCount);

        for (std::uint16_t i = 0; i < playerCount; ++i)
        {
            SnapshotPlayerState player;
            std::uint8_t rawFacing = 0;

            if (!reader.ReadU32(player.playerId) ||
                !reader.Read32(player.x) ||
                !reader.Read32(player.y) ||
                !reader.ReadU8(rawFacing) ||
                !reader.Read32(player.hp) ||
                !reader.ReadU8(player.flags))
            {
                return false;
            }

            if (!IsValidMoveDirection(rawFacing) || (player.flags & ~(PlayerStateDead | PlayerStateAttacking)) != 0)
            {
                return false;
            }

            player.facing = static_cast<MoveDirection>(rawFacing);
            parsed.players.push_back(player);
        }

        std::uint16_t enemyCount = 0;
        std::uint16_t projectileCount = 0;

        if (!reader.ReadU16(enemyCount) || !reader.ReadU16(projectileCount) || !reader.IsAtEnd())
        {
            return false;
        }

        // 현재 protocol contract.
        if (enemyCount != 0 || projectileCount != 0)
        {
            return false;
        }

        snapshot = std::move(parsed);
        return true;
    }
}
