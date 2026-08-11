#include "pch.h"
#include "DevelopmentLevel.h"
#include <Actor/CollisionTestActor.h>
#include <Render/Renderer.h>
#include <Core/Input.h>

using namespace Craft;

namespace
{
    // TEMP: 아직 Sprite가 없어 TileId 2글자 16진수 그대로 표시
    std::string MakeTileIdRow(const RoomDefinition& room, int y)
    {
        const char* const Digits = "0123456789ABCDEF";
        std::string result;
        result.reserve(RoomDefinition::Width * 2);

        for (int x = 0; x < RoomDefinition::Width; ++x)
        {
            const TileId id = room.GetTileId(x, y);
            result.push_back(Digits[(id >> 4) & 0x0f]);
            result.push_back(Digits[id & 0x0f]);
        }

        return result;
    }
}

DevelopmentLevel::DevelopmentLevel()
{
}

void DevelopmentLevel::OnInitialized()
{
    Level::OnInitialized();
}

void DevelopmentLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_attempted)
    {
        InitializeMapTest();
    }

    if (_testA && _testB)
    {
        return;
    }

    _testA = SpawnActor<CollisionTestActor>(
        Vector2(10, 10),
        Vector2::One,
        Vector2::Zero
    );

    _testB = SpawnActor<CollisionTestActor>(
        Vector2(15, 10),
        Vector2::One,
        Vector2::Zero
    );
}

void DevelopmentLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (Input::Get().GetKeyDown('M'))
    {
        _testA->SetPosition(Vector2(20, 10));
    }
}

void DevelopmentLevel::Draw()
{
    Level::Draw();

    Renderer::Get().Submit("[Development Level]", Vector2::Zero);
    Renderer::Get().Submit("Map: " + _mapStatus, Vector2(35, 1));
    Renderer::Get().Submit("CollisionCount A: " + std::to_string(_testA->GetCollisionCount()), Vector2(35, 2));
    Renderer::Get().Submit("CollisionCount B: " + std::to_string(_testB->GetCollisionCount()), Vector2(35, 3));

    if (!_room) return;

    Renderer::Get().Submit("Room (0, 0) TileIds", Vector2(0, 2));
    for (int y = 0; y < RoomDefinition::Height; ++y)
    {
        Renderer::Get().Submit(MakeTileIdRow(*_room, y), Vector2(0, 3 + y));
    }
}

void DevelopmentLevel::InitializeMapTest()
{
    _attempted = true;

    const FilePath basePath = "../Content/Z1/Maps/Overworld";
    std::string error;
    const bool loaded = _loader.Load(basePath / "TileMap.txt", basePath / "BlockingMap.txt", error);

    if (!loaded)
    {
        _mapStatus = "LOAD FAILED";
        return;
    }

    auto room = _loader.ExtractRoom(0, 0, error);
    if (!room)
    {
        _mapStatus = "ROOM EXTRACT FAILED";
        return;
    }

    _room = std::move(room);

    const bool tile = _room->GetTileId(0, 0) == 0x43;
    const bool blocked = !_room->IsWalkable(0, 0);
    const bool walkable = _room->IsWalkable(7, 0);

    if (tile && blocked && walkable)
    {
        _mapStatus = "PASS";
    }
    else
    {
        _mapStatus = "DATA MISMATCH";
    }
}
