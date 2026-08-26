#include "pch.h"
#include "CaveLevel.h"
#include <Actor/Player.h>
#include <filesystem>
#include <fstream>
#include <Render/Renderer.h>
#include <Game/Game.h>
#include <Component/BoxComponent.h>
#include <Math/Box2D.h>
#include <World/MapGeometry.h>
#include <array>

using namespace Craft;
using FilePath = std::filesystem::path;

namespace
{
    constexpr int ItemSortingOrder = 5;

    const std::vector<std::string> SwordImage
    {
        "    /\\    ",
        "    ||    ",
        "  ==++==  ",
        "    ||    ",
        "    oo    "
    };

    std::shared_ptr<const Sprite> CreateItemSprite(const std::vector<std::string> image, Color color)
    {
        const int height = image.size(), width = image[0].size();
        std::vector<SpriteCell> cells(height * width);

        for (int y = 0; y < height; ++y)
        {
            for (int x = 0; x < width; ++x)
            {
                const char glyph = image[y][x];
                if (glyph == ' ')
                {
                    continue;
                }

                // 임시로 색상 단일 색상...
                cells[y * width + x] = SpriteCell(glyph, (WORD)color, false);
            }
        }

        return std::make_shared<const Sprite>(Vector2(width, height), std::move(cells));
    }
}

void CaveLevel::OnInitialized()
{
    Level::OnInitialized();
    
    if (_loaded) return;

    _loaded = LoadMap();
    if (_loaded) BuildRoomSprite();
}

void CaveLevel::BeginPlay()
{
    Level::BeginPlay();

    if (!_loaded) return;

    Game& game = dynamic_cast<Game&>(Engine::Get());

    if (!_player)
    {
        Vector2 spawnPosition = _playerPosition * TileCellSize;
        _player = SpawnActor<Player>(spawnPosition, Game::PlayerMaxHp);
    }

    if (!_playerStateLoaded)
    {
        game.LoadPlayerState(*_player);
        _playerStateLoaded = true;
    }
}

void CaveLevel::EndPlay()
{
    Level::EndPlay();

    if (_player)
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.SavePlayerState(*_player);
    }

    _playerStateLoaded = false;
}

void CaveLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (!_player) return;

    if (_player->IsDead())
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ChangeLevel(State::GameOver);
        return;
    }

    if (UpdatePlayerMovement(deltaTime)) return;
}

void CaveLevel::Draw()
{
    Renderer& renderer = Renderer::Get();
    renderer.SetView(Vector2::Zero, Vector2(0, 3));

    if (_roomSprite)
    {
        renderer.SubmitWorld(_roomSprite, Vector2::Zero, 0);
    }

    if (!_player->HasSword() && !_swordCollected)
    {
        renderer.SubmitWorld(CreateItemSprite(SwordImage, Color::White), _swordPosition * TileCellSize, ItemSortingOrder);
    }

    Level::Draw();

    /*
    * HUD 영역
    */
    renderer.Submit(Sprite::Create("[CAVE]"), Vector2(2, 1));
    
    if (_player)
    {
        const std::string hp = "[HP " + std::to_string(_player->GetHp()) + "/" + std::to_string(_player->GetMaxHp()) + "]";
        renderer.Submit(Sprite::Create(hp), Vector2(16, 1));

        const std::string sword = _player->HasSword() ? "[SWORD]" : "[NO SWORD]";
        renderer.Submit(Sprite::Create(sword), Vector2(30, 1));
    }
}

bool CaveLevel::LoadMap()
{
    const FilePath path = "../Content/Z1/Maps/Caves/SwordCave.txt";
    std::string error;

    if (!_map.Load(path, error))
    {
        return false;
    }

    bool hasExit = false;
    bool hasSword = false;

    for (int y = 0; y < CaveMap::Height; ++y)
    {
        for (int x = 0; x < CaveMap::Width; ++x)
        {
            const char tile = _map.GetTile(x, y);

            if (tile == 'E')
            {
                if (hasExit)
                {
                    return false;
                }

                _exitPosition = Vector2(x, y);
                hasExit = true;
            }
            else if (tile == 'S')
            {
                if (hasSword)
                {
                    return false;
                }

                _swordPosition = Vector2(x, y);
                hasSword = true;
            }
        }
    }

    _playerPosition = Vector2(_exitPosition.x, _exitPosition.y - 1);
    return hasExit && hasSword && _exitPosition.y > 0;
}

void CaveLevel::BuildRoomSprite()
{
    // 단일 룸이라 origin 필요 없음
    _roomSprite = _map.BuildRoomSprite(Vector2::Zero, Vector2(RoomTileWidth, RoomTileHeight));
}

bool CaveLevel::CanMoveTo(Craft::Vector2 destination) const
{
    auto box = _player->GetComponent<BoxComponent>();
    if (!box) return false;

    return _map.CanPlaceBox(Box2D{destination + box->GetOffset(), box->GetSize()});
}

// Player Box가 특정 타일과 겹치는지 검사
bool CaveLevel::IsOnTile(Vector2 tile) const
{
    if (!_player)
    {
        return false;
    }

    const auto box = _player->GetComponent<BoxComponent>();
    if (!box)
    {
        return false;
    }

    return Box2D{_player->GetWorldPosition() + box->GetOffset(), box->GetSize()}
        .Overlaps(Box2D{tile * TileCellSize, TileCellSize}
    );
}

bool CaveLevel::UpdatePlayerMovement(float deltaTime)
{
    Vector2 delta = _player->GetMovementInputDirection();
    if (delta == Vector2::Zero) return false;

    _player->CancelAttack();
    int moveSteps = _player->ConsumeMoveSteps(deltaTime);
    for (int step = 0; step < moveSteps; ++step)
    {
        Vector2 candidate = _player->GetWorldPosition() + delta;
        if (TryExitCave(candidate, delta))
        {
            // 먼저 검사
            _player->ClearMoveRemainder();
            return true;
        }

        if (!CanMoveTo(candidate))
        {
            _player->ClearMoveRemainder();
            break;
        }

        _player->MoveBy(delta);
        TryCollectSword();
    }

    return false;
}

bool CaveLevel::TryCollectSword()
{
    if (!_player || _swordCollected) return false;

    if (!IsOnTile(_swordPosition))
    {
        return false;
    }

    _player->EquipSword();
    Engine::Get().PlayOneShot("Z1/07. Collect Item.wav");

    _swordCollected = true;

    return true;
}

bool CaveLevel::TryExitCave(Vector2 destination, Vector2 moveDelta)
{
    if (!_player) return false;

    // 아래로 내려갈 때만 출구 판정
    if (moveDelta != Vector2::Up * -1) return false;

    auto box = _player->GetComponent<BoxComponent>();
    if (!box) return false;

    Vector2 boxPos = destination + box->GetOffset();
    Vector2 boxSize = box->GetSize();

    int left = boxPos.x;
    int top = boxPos.y;
    int right = left + boxSize.x - 1;
    int bottom = top + boxSize.y - 1;

    int caveBottom = RoomTileHeight * TileCellSize.y;

    int exitLeft = _exitPosition.x * TileCellSize.x;
    int exitRight = exitLeft + 2 * TileCellSize.x - 1;  // 동굴 구조는 고정이라 2배로 설정

    bool overlapsExitWidth = left <= exitRight && right >= exitLeft;
    bool reachesCaveBoundary = bottom >= caveBottom;

    if (!overlapsExitWidth || !reachesCaveBoundary)
    {
        return false;
    }

    Game& game = dynamic_cast<Game&>(Engine::Get());
    game.ChangeLevel(State::Overworld);

    return true;
}
