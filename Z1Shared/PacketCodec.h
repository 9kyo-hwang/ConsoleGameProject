#pragma once
#include <Z1Shared/Serialization.h>
#include <Z1Shared/Protocol.h>
#include <vector>
#include <span>

namespace Z1::Protocol
{
    // Header-Only라 전부 인라인
    namespace
    {
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

    // 0 ~ 4 의 값이 아니라면 문제
    inline bool IsValidMoveDirection(std::uint8_t direction) noexcept
    {
        return direction <= (std::uint8_t)MoveDirection::Right;
    }

    // None 방향은 있을 수 없음
    inline bool IsValidCardinalDirection(std::uint8_t direction) noexcept
    {
        return direction >= (std::uint8_t)MoveDirection::Up
            && direction <= (std::uint8_t)MoveDirection::Right;
    }

    inline bool IsValidActorKind(std::uint8_t kind) noexcept
    {
        return (std::uint8_t)ActorKind::None < kind
            && kind <= (std::uint8_t)ActorKind::Projectile_Spear;
    }

    inline bool IsValidCombatEventType(std::uint8_t value) noexcept
    {
        return value == (std::uint8_t)CombatEventType::PlayerSwordAttack;   // 지금은 얘 하나만
    }

    inline bool BuildPacket_C2SEnter(std::vector<Byte>& packet)
    {
        PacketWriter payload(sizeof(std::uint16_t));
        payload.WriteU16(ProtocolVersion);

        return BuildPacket(PacketType::C2S_Enter, payload.Bytes(), packet);
    }

    inline bool ParsePayload_C2SEnter(std::span<const Byte> payload)
    {
        PacketReader reader(payload);
        std::uint16_t version = 0;

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
        std::uint32_t id = 0;
        if (!reader.ReadU16(version) || !reader.ReadU32(id) || !reader.IsAtEnd())
        {
            return false;
        }

        if (version != ProtocolVersion || id == 0)
        {
            return false;
        }

        playerId = id;
        return true;
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

    inline bool ParsePayload_C2SInput(std::span<const Byte> payload, InputCommand& out)
    {
        PacketReader reader(payload);
        InputCommand input{};
        std::uint8_t direction = 0;

        if (!reader.ReadU32(input.sequence) || !reader.ReadU8(direction) || !reader.ReadU8(input.actionFlags) || !reader.IsAtEnd())
        {
            return false;
        }

        // [11111...0]이랑 & 했는데 0이 아니라면 flag로 2 이상의 값이 들어왔다는 뜻
        if (!IsValidMoveDirection(direction) || (input.actionFlags & ~ValidInputActions) != 0)
        {
            return false;
        }

        input.moveDirection = (MoveDirection)direction;
        out = input;
        
        return true;
    }

    inline constexpr std::size_t FixedSnapshotPayloadSize 
        = sizeof(std::uint32_t) + sizeof(std::uint32_t);    // tick + count

    inline bool BuildPacket_S2CWorldSnapshot(const WorldSnapshot& snapshot, std::vector<Byte>& packet)
    {
        // 기존에 MaxPlayerCount 검증은 어떻게 대체할까

        const std::size_t payloadSize
            = FixedSnapshotPayloadSize + snapshot.actors.size() * ActorInfoSize;

        if (payloadSize > MaxPacketSize - PacketHeaderSize)
        {
            return false;
        }

        PacketWriter payload(payloadSize);
        payload.WriteU32(snapshot.serverTick);
        payload.WriteU32((std::uint32_t)snapshot.actors.size());

        for (const ActorInfo& actor : snapshot.actors)
        {
            std::uint32_t id = actor.id;
            if (id == 0) return false;

            std::uint8_t kind = (std::uint8_t)actor.kind;
            if (!IsValidActorKind(kind)) return false;

            std::uint8_t direction = (std::uint8_t)actor.direction;
            std::uint8_t flags = actor.flags;

            switch (actor.kind)
            {
            case ActorKind::Player:
            {
                // ID는 1부터 시작하도록 계약?
                if (!IsValidMoveDirection(direction) || (flags & ~(ValidPlayerState)) != 0)
                {
                    return false;
                }

                break;
            }
            case ActorKind::Enemy_Octorok:
            case ActorKind::Enemy_Moblin:
            case ActorKind::Enemy_Tektite:
            {
                // 종류 파악은 switch-case에 안걸리는 걸로 이미 수행
                if (!IsValidMoveDirection(direction) || (flags & ~(ValidEnemyState)) != 0)
                {
                    return false;
                }
                break;
            }
            case ActorKind::Projectile_Spear:
            {
                if (!IsValidCardinalDirection(direction))
                {
                    return false;
                }
                break;
            }
            default: return false;
            }

            payload.WriteU32(id);
            payload.Write32(actor.hp);
            payload.WriteU16(actor.x);
            payload.WriteU16(actor.y);
            payload.WriteU8((std::uint8_t)actor.kind);
            payload.WriteU8(direction);
            payload.WriteU8(flags);
        }

        return BuildPacket(PacketType::S2C_WorldSnapshot, payload.Bytes(), packet);
    }

    inline bool ParsePayload_S2CWorldSnapshot(std::span<const Byte> payload, WorldSnapshot& snapshot)
    {
        PacketReader reader(payload);

        // 서버 틱 + 액터 수 + ActorInfo[id, hp, x, y, kind, direction, flags] x 액터 수
        WorldSnapshot parsed;
        
        std::uint32_t actorCount = 0;
        if (!reader.ReadU32(parsed.serverTick) || !reader.ReadU32(actorCount))
        {
            return false;
        }

        // TODO: 기존 Player 1명 이상 + 적/투사체 0명 크기 검증

        parsed.actors.reserve(actorCount);
        for (std::uint32_t i = 0; i < actorCount; ++i)
        {
            ActorInfo actor;
            std::uint8_t rawKind = 0, rawDir = 0;

            if (!reader.ReadU32(actor.id) ||
                !reader.Read32(actor.hp) ||
                !reader.ReadU16(actor.x) ||
                !reader.ReadU16(actor.y) ||
                !reader.ReadU8(rawKind) ||
                !reader.ReadU8(rawDir) ||
                !reader.ReadU8(actor.flags))
            {
                return false;
            }

            if (!IsValidActorKind(rawKind))
            {
                return false;
            }

            switch ((ActorKind)rawKind)
            {
            case ActorKind::Player:
            {
                if (!IsValidMoveDirection(rawDir) || (actor.flags & ~(ValidPlayerState)) != 0)
                {
                    return false;
                }

                break;
            }
            case ActorKind::Enemy_Octorok:
            case ActorKind::Enemy_Moblin:
            case ActorKind::Enemy_Tektite:
            {
                if (!IsValidMoveDirection(rawDir) || (actor.flags & ~(ValidEnemyState)) != 0)
                {
                    return false;
                }
                break;
            }
            case ActorKind::Projectile_Spear:
            {
                if (!IsValidCardinalDirection(rawDir))
                {
                    return false;
                }
                break;
            }
            default: return false;
            }

            actor.direction = (MoveDirection)rawDir;
            actor.kind = (ActorKind)rawKind;
            parsed.actors.push_back(actor);
        }

        if (!reader.IsAtEnd())
        {
            return false;
        }

        snapshot = std::move(parsed);
        return true;
    }

    inline constexpr std::size_t CombatEventSize
        = sizeof(std::uint8_t)      // type
        + sizeof(std::uint32_t)     // actorId
        + sizeof(std::uint8_t);     // direction

    inline bool BuildPacket_S2CCombatEvent(const CombatEvent& event, std::vector<Byte>& packet)
    {
        const std::uint8_t type = (std::uint8_t)event.type;
        const std::uint8_t direction = (std::uint8_t)event.direction;

        if (!IsValidCombatEventType(type) || event.actorId == 0 || !IsValidCardinalDirection(direction))
        {
            return false;
        }

        PacketWriter payload(CombatEventSize);
        payload.WriteU8(type);
        payload.WriteU32(event.actorId);
        payload.WriteU8(direction);

        return BuildPacket(PacketType::S2C_CombatEvent, payload.Bytes(), packet);
    }

    inline bool ParsePayload_S2CCombatEvent(std::span<const Byte> payload, CombatEvent& event)
    {
        PacketReader reader(payload);
        
        CombatEvent parsed;
        std::uint8_t rawType = 0, rawDirection = 0;
        if (!reader.ReadU8(rawType) || !reader.ReadU32(parsed.actorId)
            || !reader.ReadU8(rawDirection) || !reader.IsAtEnd())
        {
            return false;
        }

        if (!IsValidCombatEventType(rawType) || !IsValidCardinalDirection(rawDirection))
        {
            return false;
        }

        parsed.type = (CombatEventType)rawType;
        parsed.direction = (MoveDirection)rawDirection;

        event = std::move(parsed);
        return true;
    }

    inline constexpr std::size_t EnemyPathDebugFixedPayloadSize
        = sizeof(std::uint32_t) // tick
        + sizeof(std::uint32_t) // id
        + sizeof(std::int32_t)  // roomX
        + sizeof(std::int32_t)  // roomY
        + sizeof(std::uint8_t); // numIndices

    inline bool BuildPacket_S2CEnemyPathDebug(const EnemyPathDebug& path, std::vector<Byte>& packet)
    {
        const std::size_t payloadSize = EnemyPathDebugFixedPayloadSize + path.tileIndices.size();
        if (payloadSize > MaxPacketSize - PacketHeaderSize)
        {
            return false;
        }

        PacketWriter payload(payloadSize);
        payload.WriteU32(path.tick);
        payload.WriteU32(path.id);
        payload.Write32(path.roomX);
        payload.Write32(path.roomY);

        const std::size_t numIndices = path.tileIndices.size();
        if (numIndices > 16 * 11) return false;

        payload.WriteU8((std::uint8_t)numIndices);
        for (std::uint8_t index : path.tileIndices)
        {
            if (index >= 16 * 11) return false;
            payload.WriteU8(index);
        }

        return BuildPacket(PacketType::S2C_EnemyPathDebug, payload.Bytes(), packet);
    }

    inline bool ParsePayload_S2CEnemyPathDebug(std::span<const Byte> payload, EnemyPathDebug& outPath)
    {
        PacketReader reader(payload);

        EnemyPathDebug parsed;
        if (!reader.ReadU32(parsed.tick) ||
            !reader.ReadU32(parsed.id) ||
            !reader.Read32(parsed.roomX) ||
            !reader.Read32(parsed.roomY))
        {
            return false;
        }

        std::uint8_t numIndices = 0;
        if (!reader.ReadU8(numIndices) || numIndices > 16 * 11)
        {
            return false;
        }

        if (reader.Remaining() != sizeof(std::uint8_t) * numIndices)
        {
            return false;
        }

        parsed.tileIndices.reserve(numIndices);
        for (std::uint8_t i = 0; i < numIndices; ++i)
        {
            std::uint8_t index = 0;
            if (!reader.ReadU8(index) || index >= 16 * 11)
            {
                return false;
            }

            parsed.tileIndices.push_back(index);
        }

        if (!reader.IsAtEnd())
        {
            return false;
        }

        outPath = std::move(parsed);
        return true;
    }
}
