#include "pch.h"
#include "NetworkEnemy.h"
#include <Component/SpriteRendererComponent.h>
#include <array>

using namespace Craft;
using namespace Z1::Protocol;

NetworkEnemy::NetworkEnemy(Vector2 position, std::uint32_t id, ActorKind kind)
    : Super(position)
    , _id(id)
    , _kind(kind)
{
    _renderer = AddComponent<SpriteRendererComponent>(CreateSprite(), 10);
}

void NetworkEnemy::ApplySnapshot(const Z1::Protocol::ActorInfo& info)
{
    // 같은 ID에 다른 kind?

    assert(_id == info.id);
    assert(_kind == info.kind);

    if (_id != info.id || _kind != info.kind) return;

    SetPosition(Vector2(info.x, info.y));
    _direction = info.direction;
    _hp = info.hp;
    _flags = info.flags;
}

std::shared_ptr<const Craft::Sprite> NetworkEnemy::CreateSprite()
{
    const std::array<std::string, 5> art
    {
        "   /\\   ",
        "/######\\",
        "<##@@##>",
        "\\######/",
        " /####\\ "
    };

    std::vector<SpriteCell> cells;
    cells.reserve(40);

    Color bodyColor = Color::Black;
    switch (_kind)
    {
    case ActorKind::Enemy_Octorok: bodyColor = Color::DarkRed; break;
    case ActorKind::Enemy_Moblin:  bodyColor = Color::DarkBlue; break;
    case ActorKind::Enemy_Tektite: bodyColor = Color::DarkViolet; break;
    }

    for (const std::string& row : art)
    {
        for (const char glyph : row)
        {
            Color color = bodyColor;
            if (glyph == '@')
            {
                color = Color::Yellow;
            }
            else if (glyph == '/' || glyph == '\\' || glyph == '<' || glyph == '>')
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

    return std::make_shared<const Sprite>(
        Vector2(8, 5),
        std::move(cells)
    );
}
