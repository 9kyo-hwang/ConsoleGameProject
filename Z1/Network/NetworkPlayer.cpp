#include "pch.h"
#include "NetworkPlayer.h"
#include <Component/SpriteRendererComponent.h>
#include <array>
#include <Engine/Engine.h>

using namespace Craft;
using namespace Z1::Protocol;

NetworkPlayer::NetworkPlayer(Vector2 position, std::uint32_t id)
    : Super(position)
    , _id(id)
{
    _renderer = AddComponent<SpriteRendererComponent>(CreateSprite(), 10);

    // 서버에서 충돌을 결정하므로 로컬 충돌 판정에 끼면 안됨(BoxCollider 없음)
}

/// <summary>
/// 서버로부터 아래 상태 갱신
/// - 방향, Sprite 표현
/// - HP
/// - 공격 중 Flag
/// - 사망 Flag
/// </summary>
/// <param name="state">서버의 원본 Snapshot</param>
void NetworkPlayer::ApplySnapshot(const Z1::Protocol::ActorInfo& info)
{
    assert(_id == info.id);
    if (_id != info.id)
    {
        return;
    }

    // Snapshot 적용 전, 이전 상태 보관
    bool hadSnapshot = _hasSnapshot;
    std::int32_t prevHp = _hp;
    bool wasDead = IsDead();

    // 새로운 상태로 갱신
    SetPosition(Vector2(info.x, info.y));
    _direction = info.direction;
    _hp = info.hp;
    _flags = info.flags;

    // 최초 1회에는 안들어옴
    if (hadSnapshot)
    {
        if (!wasDead && IsDead())
        {
            Engine::Get().PlayOneShot("Z1/LOZ_Link_Die.wav");
        }
        else if (info.hp < prevHp)
        {
            Engine::Get().PlayOneShot("Z1/LOZ_Link_Hurt.wav");
        }
    }

    _hasSnapshot = true;
}

std::shared_ptr<const Craft::Sprite> NetworkPlayer::CreateSprite()
{
    const std::array<std::string, 5> art
    {
        "   /\\   ",
        "  /@@\\  ",
        "<######>",
        " /####\\ ",
        "   ||   "
    };

    std::vector<SpriteCell> cells; cells.reserve(40);

    for (const std::string& row : art)
    {
        for (const char glyph : row)
        {
            Color color = Color::DarkViolet;
            if (glyph == '@')
            {
                color = Color::White;
            }
            else if (glyph == '/' || glyph == '\\' || glyph == '<' || glyph == '>')
            {
                color = Color::DarkGreen;
            }
            else if (glyph == '|')
            {
                color = Color::DarkYellow;
            }

            cells.emplace_back(SpriteCell
                {
                    glyph,
                    static_cast<WORD>(color),
                    glyph == ' '
                });
        }
    }

    return std::make_shared<const Sprite>(Vector2(8, 5), std::move(cells));
}
