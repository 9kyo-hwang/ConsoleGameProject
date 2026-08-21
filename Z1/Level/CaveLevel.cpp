#include "pch.h"
#include "CaveLevel.h"
#include <Actor/Player.h>
#include <filesystem>
#include <fstream>
#include <Render/Renderer.h>
#include <Game/Game.h>
#include <Component/BoxComponent.h>
#include <array>

using namespace Craft;
using FilePath = std::filesystem::path;

namespace
{
    constexpr int ItemTileWidth = 10;
    constexpr int ItemTileHeight = 5;

    void DrawSwordSprite(
        std::vector<SpriteCell>& cells,
        int spriteWidth,
        int tileX,
        int tileY)
    {
        const std::array<std::string, 5> art
        {
            "    /\\    ",
            "    ||    ",
            "  ==++==  ",
            "    ||    ",
            "    oo    "
        };

        for (int y = 0; y < static_cast<int>(art.size()); ++y)
        {
            for (int x = 0; x < static_cast<int>(art[y].size()); ++x)
            {
                const char glyph = art[y][x];
                if (glyph == ' ')
                {
                    continue;
                }

                Color color = Color::White;
                if (glyph == '=' || glyph == '+')
                {
                    color = Color::Yellow;
                }
                else if (glyph == 'o')
                {
                    color = Color::DarkYellow;
                }

                const int destX = tileX * ItemTileWidth + x;
                const int destY = tileY * ItemTileHeight + y;

                cells[destY * spriteWidth + destX] = SpriteCell(
                    glyph,
                    static_cast<WORD>(color),
                    false
                );
            }
        }
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

    if (_player)
    {
        if (_needsPlayerSync)
        {
            _player->SetHealth(game.GetPlayerHp());
            _needsPlayerSync = false;
        }

        if (game.HasSword())
        {
            _player->EquipSword();
        }

        return;
    }

    Vector2 spawnPosition(_playerPosition.x * TilePixelWidth, _playerPosition.y * TilePixelHeight);
    _player = SpawnActor<Player>(spawnPosition, 10);
    _player->SetHealth(game.GetPlayerHp());
    _needsPlayerSync = false;

    // game??
    if (game.HasSword())
    {
        _player->EquipSword();
        _swordCollected = true;
        BuildRoomSprite();  // S 없이 새롭게 그리기. 근데 그냥 Sword를 액터처럼 해서 destroy하는 게 안낫나?
    }
}

void CaveLevel::EndPlay()
{
    Level::EndPlay();

    if (_player)
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.SetPlayerHp(_player->GetHp());
    }

    _needsPlayerSync = true;
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

    Level::Draw();

    renderer.Submit("[CAVE]", Vector2(2, 1));
    
    if (_player)
    {
        const std::string hp = "[HP " + std::to_string(_player->GetHp()) + "/" + std::to_string(_player->GetMaxHp()) + "]";
        renderer.Submit(hp, Vector2(16, 1));

        const std::string sword = _player->HasSword() ? "[SWORD]" : "[NO SWORD]";
        renderer.Submit(sword, Vector2(30, 1));
    }
}

bool CaveLevel::LoadMap()
{
    const FilePath path = "../Content/Z1/Maps/Caves/SwordCave.txt";
    
    std::ifstream file(path);
    if (!file.is_open())
    {
        return false;
    }

    bool hasExit = false;
    bool hasSword = false;

    std::string line;
    for (int y = 0; y < RoomHeight; ++y)
    {
        if (!std::getline(file, line))
        {
            return false;
        }

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.size() != RoomWidth)
        {
            return false;
        }

        for (int x = 0; x < RoomWidth; ++x)
        {
            char symbol = line[x];
            if (symbol != '#' && symbol != '.' && symbol != 'E' && symbol != 'S')
            {
                return false;
            }

            _grid[y][x] = symbol;

            if (symbol == 'E')
            {
                if (hasExit)
                {
                    return false;
                }

                _exitPosition = Vector2(x, y);
                hasExit = true;
            }
            else if (symbol == 'S')
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

    if (std::getline(file, line)) return false;
    if (!hasExit || !hasSword) return false;
    if (_exitPosition.y <= 0) return false;

    _playerPosition = Vector2(_exitPosition.x, _exitPosition.y - 1);
    return true;
}

void CaveLevel::BuildRoomSprite()
{
    const Vector2 spriteSize(RoomWidth * TilePixelWidth, RoomHeight * TilePixelHeight);
    std::vector<SpriteCell> cells(spriteSize.x * spriteSize.y);

    for (int tileY = 0; tileY < RoomHeight; ++tileY)
    {
        for (int tileX = 0; tileX < RoomWidth; ++tileX)
        {
            char symbol = _grid[tileY][tileX];
            SpriteCell visual;
            if (symbol == '#')
            {
                visual = SpriteCell(symbol, (WORD)Color::DarkRed, false);
            }
            else
            {
                visual = SpriteCell(' ', (WORD)Color::Black, false);
            }

            for (int offsetY = 0; offsetY < TilePixelHeight; ++offsetY)
            {
                for (int offsetX = 0; offsetX < TilePixelWidth; ++offsetX)
                {
                    int x = tileX * TilePixelWidth + offsetX;
                    int y = tileY * TilePixelHeight + offsetY;

                    cells[y * spriteSize.x + x] = visual;
                }
            }

            if (symbol == 'S' && !_swordCollected)
            {
                DrawSwordSprite(cells, spriteSize.x, tileX, tileY);
            }
        }
    }

    _roomSprite = std::make_shared<const Sprite>(spriteSize, std::move(cells));
}

bool CaveLevel::CanMoveTo(Craft::Vector2 destination) const
{
    auto box = _player->GetComponent<BoxComponent>();
    if (!box) return false;

    Vector2 boxPos = destination + box->GetOffset();
    Vector2 boxSize = box->GetSize();

    int left = boxPos.x;
    int top = boxPos.y;
    int right = left + boxSize.x - 1;
    int bottom = top + boxSize.y - 1;

    int minTileX = left / TilePixelWidth;
    int minTileY = top / TilePixelHeight;
    int maxTileX = right / TilePixelWidth;
    int maxTileY = bottom / TilePixelHeight;


    if (minTileX < 0 || minTileY < 0 || maxTileX >= RoomWidth || maxTileY >= RoomHeight)
    {
        return false;
    }

    for (int tileY = minTileY; tileY <= maxTileY; ++tileY)
    {
        for (int tileX = minTileX; tileX <= maxTileX; ++tileX)
        {
            if (_grid[tileY][tileX] == '#')
            {
                return false;
            }
        }
    }

    return true;
}

// Player가 특정 tile 위에 있는지(추후 Sprite 교체되면 box 활용)
bool CaveLevel::IsOnTile(Craft::Vector2 tile) const
{
    Vector2 position = _player->GetWorldPosition();
    int tileX = position.x / TilePixelWidth;
    int tileY = position.y / TilePixelHeight;

    return tileX == tile.x && tileY == tile.y;
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

    Game& game = dynamic_cast<Game&>(Engine::Get());
    if (game.HasSword())
    {
        // GAME??
        _swordCollected = true;
        return false;
    }

    if (!IsOnTile(_swordPosition))
    {
        return false;
    }

    game.SetHasSword(true);
    _player->EquipSword();

    _swordCollected = true;
    BuildRoomSprite();

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

    int caveBottom = RoomHeight * TilePixelHeight;

    int exitLeft = _exitPosition.x * TilePixelWidth;
    int exitRight = exitLeft + 2 * TilePixelWidth - 1;  // 동굴 구조는 고정이라 2배로 설정

    bool overlapsExitWidth = left <= exitRight && right >= exitLeft;
    bool reachesCaveBoundary = bottom >= caveBottom;

    if (!overlapsExitWidth || !reachesCaveBoundary)
    {
        return false;
    }

    Game& game = dynamic_cast<Game&>(Engine::Get());
    game.SetPlayerHp(_player->GetHp());
    game.SetHasSword(_player->HasSword());
    game.ChangeLevel(State::Overworld);

    return true;
}
