#include "pch.h"
#include "NetworkOverworldLevel.h"

#include <Render/Renderer.h>
#include <Game/Game.h>
#include <Core/Input.h>
#include <unordered_set>
#include <filesystem>
#include <World/MapGeometry.h>

#include <Network/NetworkPlayer.h>
#include <Network/MyPlayer.h>
#include <Network/NetworkEnemy.h>
#include <Network/NetworkProjectile.h>
#include <Network/NetworkSwordEffect.h>

using namespace Craft;
using namespace Z1::Protocol;

namespace
{
    // Map 기준 Room의 좌상단 셀 좌표
    Vector2 GetRoomCellOrigin(RoomCoordinate room)
    {
        return Vector2
        {
            room.x * RoomTileWidth * TileCellSize.x,
            room.y * RoomTileHeight * TileCellSize.y
        };
    }

    /// <summary>
    /// 현재 위치(전체 맵에 Cell 좌표 기준)가 어디 Room에 속하는지
    /// </summary>
    RoomCoordinate GetRoomCoordinate(Vector2 mapCellPosition)
    {
        assert(mapCellPosition.x >= 0 && mapCellPosition.y >= 0);

        // Room의 셀 단위 가로/세로 길이
        const int roomCellWidth = RoomTileWidth * TileCellSize.x;
        const int roomCellHeight = RoomTileHeight * TileCellSize.y;

        return RoomCoordinate(mapCellPosition.x / roomCellWidth, mapCellPosition.y / roomCellHeight);
    }
}

NetworkOverworldLevel::NetworkOverworldLevel()
{

}

void NetworkOverworldLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_loaded)
    {
        LoadMap();
    }

    if (!_loaded)
    {
        return;
    }

    Game& game = dynamic_cast<Game&>(Engine::Get());

    if (!_bgmStarted)
    {
        Engine::Get().PlayBGM("Z1/02. Overworld of Hyrule.wav");
        _bgmStarted = true;
    }
}

void NetworkOverworldLevel::Tick(float deltaTime)
{
    Game& game = dynamic_cast<Game&>(Engine::Get());
    game.PumpNetwork(); // OverworldLevel에서, 메인 스레드가 네트워크 큐를 소비하도록

    if (!game.IsServerConnected())
    {
        game.OnDisconnect("서버와의 연결이 끊어졌습니다.");
        Clear();
        return;
    }

    if (Input::Get().GetKeyDown(VK_F3))
    {
        _showEnemyPathDebug = !_showEnemyPathDebug;
    }

    UpdateSnapshot(game);   // 서버로부터 받은 정보를 Actor::Tick 보다 먼저 반영
    for (CombatEvent combatEvent : game.ConsumeCombatEvents())
    {
        ApplyCombatEvent(combatEvent);
    }

    Level::Tick(deltaTime); // MyPlayer 입력 전송 및 NetworkPlayer Actor 갱신

    if (_myPlayer && _myPlayer->IsDead())
    {
        // TEMP. 쫓아내지말고 respawn을 시도해볼까?
        game.OnDisconnect("플레이어가 사망하였습니다.");
        Clear();
        return;
    }
}

void NetworkOverworldLevel::Draw()
{
    Renderer& renderer = Renderer::Get();
    Game& game = dynamic_cast<Game&>(Engine::Get());

    const Vector2 roomCellOrigin = GetRoomCellOrigin(_currentRoom);
    renderer.SetView(roomCellOrigin, RoomScreenOffset);

    if (_roomSprite)
    {
        renderer.SubmitWorld(_roomSprite, roomCellOrigin, 0);
    }

    Level::Draw();

    if (_showEnemyPathDebug)
    {
        DrawLatestEnemyPathDebug(game, renderer, roomCellOrigin);
    }

    renderer.Submit(Sprite::Create("[Overworld]"), Vector2(2, 1));

    if (_myPlayer)
    {
        // 최대 체력은 추후 SnapshotPlayerState에 MaxHp 수치도 넣고, 다 바꾸도록...
        const std::string hp = "[HP " + std::to_string(_myPlayer->GetHp()) + "/" + std::to_string(Game::PlayerMaxHp) + "]";
        renderer.Submit(Sprite::Create(hp), Vector2(16, 1));
    }

    const auto localPlayerId = game.GetLocalPlayerId();
    const auto& snapshot = game.GetLatestSnapshot();
    const Vector2 NetHUDPos(46, 1);
    std::string netText;

    if (!localPlayerId.has_value())
    {
        netText = "[Connecting...]";
    }
    else if (!snapshot.has_value())
    {
        // PlayerId는 받았는데 snapshot 받기 전
        netText = "[ONLINE] Player " + std::to_string(*localPlayerId);
    }
    else
    {
        netText = "[ONLINE] Player " + std::to_string(*localPlayerId)
            + " Tick " + std::to_string(snapshot->serverTick);
    }

    renderer.Submit(Sprite::Create(netText), NetHUDPos);
}

void NetworkOverworldLevel::UpdateSnapshot(Game& game)
{
    auto localPlayerId = game.GetLocalPlayerId();
    const auto& snapshot = game.GetLatestSnapshot();

    if (!localPlayerId || !snapshot)
    {
        return;
    }

    // game의 snapshot은 단순히 마지막 값을 기록하는 용도라서 serverTick 기준 진짜 최신값인지 확인해야 함
    if (_lastAppliedServerTick && *_lastAppliedServerTick == snapshot->serverTick)
    {
        return;
    }

    // 1. 서버로부터 받은 플레이어/적/투사체 정보 기반 생성 및 갱신
    std::unordered_set<std::uint32_t> players, enemies, projectiles;
    for (const ActorInfo& actor : snapshot->actors)
    {
        switch (actor.kind)
        {
        case ActorKind::Player:
        {
            if (actor.id == *localPlayerId)
            {
                if (!_myPlayer)
                {
                    _myPlayer = SpawnActor<MyPlayer>(Vector2(actor.x, actor.y), actor.id);
                }

                _myPlayer->ApplySnapshot(actor);
                TryChangeRoom(actor);    // MyPlayer 한정으로 Room 변경
            }
            else
            {
                // 원격 플레이어는 생성 or 갱신
                players.emplace(actor.id);
                auto found = _networkPlayers.find(actor.id);
                if (found != _networkPlayers.end())
                {
                    found->second->ApplySnapshot(actor);
                    continue;
                }

                auto remotePlayer = SpawnActor<NetworkPlayer>(Vector2(actor.x, actor.y), actor.id);
                remotePlayer->ApplySnapshot(actor);
                _networkPlayers.emplace(actor.id, std::move(remotePlayer));
            }
            break;
        }
        case ActorKind::Enemy_Octorok:
        case ActorKind::Enemy_Moblin:
        case ActorKind::Enemy_Tektite:
        {
            // 원격 플레이어는 생성 or 갱신
            enemies.emplace(actor.id);
            auto found = _networkEnemies.find(actor.id);
            if (found != _networkEnemies.end())
            {
                found->second->ApplySnapshot(actor);
                continue;
            }

            auto remoteEnemy = SpawnActor<NetworkEnemy>(Vector2(actor.x, actor.y), actor.id, actor.kind);
            remoteEnemy->ApplySnapshot(actor);
            _networkEnemies.emplace(actor.id, std::move(remoteEnemy));
            break;
        }
        case ActorKind::Projectile_Spear:
        {
            // 원격 플레이어는 생성 or 갱신
            projectiles.emplace(actor.id);
            auto found = _networkProjectiles.find(actor.id);
            if (found != _networkProjectiles.end())
            {
                found->second->ApplySnapshot(actor);
                continue;
            }

            // 최초 생성 시 Sound도 재생하도록 변경
            auto remoteProjectile = SpawnActor<NetworkProjectile>(Vector2(actor.x, actor.y), actor.id, actor.kind, actor.direction);
            remoteProjectile->ApplySnapshot(actor);
            
            // Room 넘어갈 때, 다른 요인에 의해 이미 생성된 projectile도 여기에 진입할 수 있음
            // 이 경우 최초 snapshot 또는 room 전환 snapshot으로 엄밀히 비교해야 함
            _networkProjectiles.emplace(actor.id, std::move(remoteProjectile));
            break;
        }
        default: return;
        }
    }

    // 2. 새로 받은 플레이어가 기존 플레이어 명단에 없다면 제거
    for (auto it = _networkPlayers.begin(); it != _networkPlayers.end();)
    {
        if (!players.contains(it->first))
        {
            it->second->Destroy();
            it = _networkPlayers.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 3. 새로 받은 적이 기존 적 명단에 없다면 제거
    for (auto it = _networkEnemies.begin(); it != _networkEnemies.end();)
    {
        if (!enemies.contains(it->first))
        {
            it->second->Destroy();
            it = _networkEnemies.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 4. 새로 받은 투사체가 기존 투사체 명단에 없다면 제거
    for (auto it = _networkProjectiles.begin(); it != _networkProjectiles.end();)
    {
        if (!projectiles.contains(it->first))
        {
            it->second->Destroy();
            it = _networkProjectiles.erase(it);
        }
        else
        {
            ++it;
        }
    }

    // 서버로부터 정상적으로 스냅샷 수신한 틱을 기록해 최신값 판별
    _lastAppliedServerTick = snapshot->serverTick;
}

void NetworkOverworldLevel::ApplyCombatEvent(const CombatEvent& event)
{
    if (event.type != CombatEventType::PlayerSwordAttack) return;

    // 나 아니면 타 클라
    std::shared_ptr<NetworkPlayer> source;
    if (_myPlayer && _myPlayer->GetPlayerId() == event.actorId)
    {
        source = _myPlayer;
    }
    else
    {
        auto found = _networkPlayers.find(event.actorId);
        if (found != _networkPlayers.end())
        {
            source = found->second;
        }
    }

    if (!source) return;

    auto effect = SpawnActor<NetworkSwordEffect>(event.direction);
    effect->AttachTo(source, false);
    Engine::Get().PlayOneShot("Z1/LOZ_Sword_Slash.wav");
}

void NetworkOverworldLevel::Clear()
{
    if (_myPlayer)
    {
        _myPlayer->Destroy();
        _myPlayer.reset();
    }

    for (auto& [id, player] : _networkPlayers)
    {
        player->Destroy();
    }

    _networkPlayers.clear();

    for (auto& [id, enemy] : _networkEnemies)
    {
        enemy->Destroy();
    }

    _networkEnemies.clear();

    for (auto& [id, projectile] : _networkProjectiles)
    {
        projectile->Destroy();
    }

    _networkProjectiles.clear();
    _lastAppliedServerTick.reset();
}

bool NetworkOverworldLevel::LoadMap()
{
    using FilePath = std::filesystem::path;

    const FilePath basePath = "../Content/Z1/Maps/Overworld";
    std::string error;

    if (!_map.Load(basePath / "TileMap.txt", basePath / "BlockingMap.txt", error))
    {
        _loaded = false;
        return false;
    }

    _currentRoom = StartRoom;
    BuildRoomSprite();

    _loaded = true;
    return true;
}

void NetworkOverworldLevel::TryChangeRoom(const ActorInfo& playerInfo)
{
    const RoomCoordinate nextRoom = GetRoomCoordinate(Vector2(playerInfo.x, playerInfo.y));
    if (nextRoom == _currentRoom) return;

    _currentRoom = nextRoom;
    BuildRoomSprite();
}

void NetworkOverworldLevel::BuildRoomSprite()
{
    /*
    * 현재 Room 좌표에 해당하는 부분 Sprite를
    * Map에 질의해 가져오자
    */

    // 논리 타일 위치: (16, 11) x (7, 7)
    const Vector2 roomOrigin(_currentRoom.x * RoomTileWidth, _currentRoom.y * RoomTileHeight);
    _roomSprite = _map.BuildRoomSprite(roomOrigin, Vector2(RoomTileWidth, RoomTileHeight));
}

void NetworkOverworldLevel::DrawLatestEnemyPathDebug(Game& game, Renderer& renderer, const Vector2 roomCellOrigin)
{
    const auto sprite = Sprite::Create(TileCellSize, '+', Color::DarkViolet);
    for (const auto& [id, debugPath] : game.GetLatestEnemyPathDebugs())
    {
        if (RoomCoordinate{ debugPath.roomX, debugPath.roomY } != _currentRoom)
        {
            continue;
        }

        for (std::uint8_t index : debugPath.tileIndices)
        {
            // (y: 3, x: 7) -> 3 * 16 + 16 = 55 | 55 / 16 = 3, 55 % 16 = 7, 
            // y * width + x
            const int localY = index / RoomTileWidth;
            const int localX = index % RoomTileWidth;
            const Vector2 worldPosition = roomCellOrigin + Vector2(localX, localY) * TileCellSize;

            renderer.SubmitWorld(sprite, worldPosition, 5);
        }
    }
}
