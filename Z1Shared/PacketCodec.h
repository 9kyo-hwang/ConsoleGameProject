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

    inline bool IsValidEnemyKind(std::uint8_t kind) noexcept
    {
        return kind <= (std::uint8_t)EnemyKind::Tektite;
    }

    inline bool IsValidProjectileKind(std::uint8_t kind) noexcept
    {
        return kind <= (std::uint8_t)ProjectileKind::Spear;
    }

    // None 방향은 있을 수 없음
    inline bool IsValidCardinalDirection(std::uint8_t direction) noexcept
    {
        return direction >= (std::uint8_t)MoveDirection::Up
            && direction <= (std::uint8_t)MoveDirection::Right;
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

    inline constexpr std::size_t SnapshotPlayerStateSize 
        = sizeof(std::uint32_t) // id 
        + sizeof(std::int32_t)  // x
        + sizeof(std::int32_t)  // y
        + sizeof(std::uint8_t)  // facing
        + sizeof(std::int32_t)  // hp
        + sizeof(std::uint8_t); // flags

    inline constexpr std::size_t SnapshotEnemyStateSize
        = sizeof(std::uint32_t) // id 
        + sizeof(std::uint8_t)  // kind
        + sizeof(std::int32_t)  // x
        + sizeof(std::int32_t)  // y
        + sizeof(std::uint8_t)  // facing
        + sizeof(std::int32_t)  // hp
        + sizeof(std::uint8_t); // flags

    inline constexpr std::size_t SnapshotProjectileStateSize
        = sizeof(std::uint32_t) // id 
        + sizeof(std::uint8_t)  // kind
        + sizeof(std::int32_t)  // x
        + sizeof(std::int32_t)  // y
        + sizeof(std::uint8_t); // facing

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

        const std::size_t payloadSize
            = WorldSnapshotFixedPayloadSize // tick + playerCnt + enemyCnt + projCnt
            + snapshot.players.size() * SnapshotPlayerStateSize
            + snapshot.enemies.size() * SnapshotEnemyStateSize
            + snapshot.projectiles.size() * SnapshotProjectileStateSize;

        if (payloadSize > MaxPacketSize - PacketHeaderSize)
        {
            return false;
        }

        PacketWriter payload(payloadSize);
        payload.WriteU32(snapshot.serverTick);

        payload.WriteU16((std::uint16_t)snapshot.players.size());
        for (const SnapshotPlayerState& player : snapshot.players)
        {
            const std::uint8_t facing = (std::uint8_t)player.facing;
            if (!IsValidMoveDirection(facing) || (player.flags & ~(ValidPlayerState)) != 0)
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

        payload.WriteU16((std::uint16_t)snapshot.enemies.size());    // enemy
        for (const SnapshotEnemyState& enemy : snapshot.enemies)
        {
            const std::uint8_t facing = (std::uint8_t)enemy.facing;
            const std::uint8_t kind = (std::uint8_t)enemy.kind;
            if (enemy.id == 0 
                || !IsValidMoveDirection(facing) 
                || !IsValidEnemyKind(kind) 
                || (enemy.flags & ~(ValidEnemyState)) != 0)
            {
                return false;
            }

            payload.WriteU32(enemy.id);
            payload.WriteU8(kind);
            payload.Write32(enemy.x);
            payload.Write32(enemy.y);
            payload.WriteU8(facing);
            payload.Write32(enemy.hp);
            payload.WriteU8(enemy.flags);
        }

        payload.WriteU16((std::uint16_t)snapshot.projectiles.size());    // projectile
        for (const SnapshotProjectileState& projectile : snapshot.projectiles)
        {
            const std::uint8_t direction = (std::uint8_t)projectile.direction;
            const std::uint8_t kind = (std::uint8_t)projectile.kind;
            if (projectile.id == 0 || !IsValidCardinalDirection(direction) || !IsValidProjectileKind(kind))
            {
                return false;
            }

            payload.WriteU32(projectile.id);
            payload.WriteU8(kind);
            payload.Write32(projectile.x);
            payload.Write32(projectile.y);
            payload.WriteU8(direction);
        }

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

        // 최소한 [players...][enemyCount(0)][projectileCount(0)] 만큼은 있어야 함
        constexpr std::size_t CountTailSize = sizeof(std::uint16_t) + sizeof(std::uint16_t);
        const std::size_t requiredPlayersBytes = (std::size_t)playerCount * SnapshotPlayerStateSize;
        
        if (reader.Remaining() < requiredPlayersBytes + CountTailSize
            || playerCount > MaxSnapshotPlayers)
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

            if (!IsValidMoveDirection(rawFacing) || (player.flags & ~(ValidPlayerState)) != 0)
            {
                return false;
            }

            player.facing = static_cast<MoveDirection>(rawFacing);
            parsed.players.push_back(player);
        }

        std::uint16_t enemyCount = 0;
        if (!reader.ReadU16(enemyCount))
        {
            return false;
        }

        const std::size_t requiredEnemiesBytes = (std::size_t)enemyCount * SnapshotEnemyStateSize;
        if (reader.Remaining() < requiredEnemiesBytes + sizeof(std::uint16_t))  // projectile count
        {
            return false;
        }

        parsed.enemies.reserve(enemyCount);
        for (std::uint16_t i = 0; i < enemyCount; ++i)
        {
            SnapshotEnemyState enemy;
            std::uint8_t rawKind = 0, rawFacing = 0;

            if (!reader.ReadU32(enemy.id) ||
                !reader.ReadU8(rawKind) ||
                !reader.Read32(enemy.x) ||
                !reader.Read32(enemy.y) ||
                !reader.ReadU8(rawFacing) ||
                !reader.Read32(enemy.hp) ||
                !reader.ReadU8(enemy.flags))
            {
                return false;
            }

            if (enemy.id == 0 || !IsValidMoveDirection(rawFacing) || !IsValidEnemyKind(rawKind) || (enemy.flags & ~(ValidEnemyState)) != 0)
            {
                return false;
            }

            enemy.kind = (EnemyKind)rawKind;
            enemy.facing = (MoveDirection)rawFacing;
            parsed.enemies.push_back(enemy);
        }

        std::uint16_t projectileCount = 0;
        if (!reader.ReadU16(projectileCount))
        {
            return false;
        }

        const std::size_t requiredProjectileBytes = (std::size_t)projectileCount * SnapshotProjectileStateSize;
        if (reader.Remaining() != requiredProjectileBytes)  
        {
            return false;
        }

        parsed.projectiles.reserve(projectileCount);
        for (std::uint16_t i = 0; i < projectileCount; ++i)
        {
            SnapshotProjectileState projectile;
            std::uint8_t rawKind = 0, rawDirection = 0;

            if (!reader.ReadU32(projectile.id) ||
                !reader.ReadU8(rawKind) ||
                !reader.Read32(projectile.x) ||
                !reader.Read32(projectile.y) ||
                !reader.ReadU8(rawDirection))
            {
                return false;
            }

            if (projectile.id == 0 || !IsValidCardinalDirection(rawDirection) || !IsValidProjectileKind(rawKind))
            {
                return false;
            }

            projectile.kind = (ProjectileKind)rawKind;
            projectile.direction = (MoveDirection)rawDirection;
            parsed.projectiles.push_back(projectile);
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
        const std::size_t type = (std::uint8_t)event.type;
        const std::size_t direction = (std::uint8_t)event.direction;

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
