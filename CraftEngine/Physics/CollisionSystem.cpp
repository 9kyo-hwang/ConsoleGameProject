#include "pch.h"
#include "CollisionSystem.h"
#include <Actor/Actor.h>

void Craft::CollisionSystem::ProcessCollision(const std::vector<std::shared_ptr<Actor>> actors)
{
    if (actors.empty())
    {
        return;
    }

    std::vector<CollisionPair> collidedActors;
    const int count = (int)actors.size();

    for (int i = 0; i < count; ++i)
    {
        const auto& lhs = actors[i];
        if (!lhs || !lhs->IsActive())
        {
            continue;
        }

        for (int j = i + 1; j < count; ++j)
        {
            const auto& rhs = actors[j];
            if (!rhs || !rhs->IsActive())
            {
                continue;
            }

            if (IsCollide(lhs, rhs))
            {
                collidedActors.emplace_back(lhs, rhs);
            }
        }
    }

    if (collidedActors.empty())
    {
        return;
    }

    for (const auto& [lhs, rhs] : collidedActors)
    {
        // 앞서 다른 쌍과의 검사로 이미 비활성화된 경우 pass
        if (!lhs->IsActive() || !rhs->IsActive())
        {
            continue;
        }

        lhs->OnCollision(rhs);
        rhs->OnCollision(lhs);
    }
}

bool Craft::CollisionSystem::IsCollide(const std::shared_ptr<Actor>& lhs, const std::shared_ptr<Actor>& rhs)
{
    if (!lhs || !lhs->IsActive() || !rhs || !rhs->IsActive())
    {
        return false;
    }

    /*
    * AABB(Axis-Aligned Bounding Box) Collision
    * float로 계산한 걸 int 좌표로 표현하다보니, 
    * 빠르게 이동하는 물체는 한 프레임에 몇 칸을 넘어갈 수 있음.
    * 이 경우 이동 경로 중간에 통과할 수 없는 물체(적 등)가 있어도
    * 무시하고 건너뛰어지는 경우가 종종 발생.
    * 따라서 이전 프레임 위치 정보를 이용, 이전 위치 ~ 현재 위치를 아우르는 긴 박스를 만들고
    * 이 박스와 충돌한 액터가 있는지 판정하는 식으로 보강
    */

    const Vector2 lhsPrev = lhs->GetPreviousPosition();
    const Vector2 lhsCur = lhs->GetPosition();

    const Vector2 rhsPrev = rhs->GetPreviousPosition();
    const Vector2 rhsCur = rhs->GetPosition();

    // X축 판정
    const int32 lhsXMin = std::min<int32>(lhsPrev.x, lhsCur.x);
    const int32 lhsXMax = std::max<int32>(lhsPrev.x, lhsCur.x) + lhs->GetWidth() - 1;

    const int32 rhsXMin = std::min<int32>(rhsPrev.x, rhsCur.x);
    const int32 rhsXMax = std::max<int32>(rhsPrev.x, rhsCur.x) + rhs->GetWidth() - 1;

    if (lhsXMax < rhsXMin || rhsXMax < lhsXMin)
    {
        return false;
    }

    // Y축 판정
    const int32 lhsYMin = std::min<int32>(lhsPrev.y, lhsCur.y);
    const int32 lhsYMax = std::max<int32>(lhsPrev.y, lhsCur.y);

    const int32 rhsYMin = std::min<int32>(rhsPrev.y, rhsCur.y);
    const int32 rhsYMax = std::max<int32>(rhsPrev.y, rhsCur.y);

    if (lhsYMax < rhsYMin || rhsYMax < lhsYMin)
    {
        return false;
    }

    // 충돌 Yes
    return true;
}
