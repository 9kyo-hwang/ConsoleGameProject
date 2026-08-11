#include "pch.h"
#include "CollisionSystem.h"
#include <Actor/Actor.h>
#include <Component/BoxComponent.h>

namespace Craft
{
    namespace
    {
        // 한 프레임동안 Actor의 Box가 차지했거나 차지할 가능성이 있는 전체 직사각형 영역
        // 충돌 검사에 사용되는 임시 데이터
        struct SweptBounds
        {
            int minX;
            int maxX;
            int minY;
            int maxY;
        };

        // SweptBounds로 한 Actor의 충돌 범위를 먼저 계산하고 두 범위를 비교하도록
        SweptBounds CalculateSweptBounds(const Actor& actor, const BoxComponent& box)
        {
            const Vector2 previous = actor.GetPreviousPosition();
            const Vector2 current = actor.GetWorldPosition();
            
            const Vector2 size = box.GetSize();
            const Vector2 offset = box.GetOffset();

            int minX = std::min<int>(previous.x, current.x) + offset.x;
            int maxX = std::max<int>(previous.x, current.x) + offset.x + size.x - 1;
            int minY = std::min<int>(previous.y, current.y) + offset.y;
            int maxY = std::max<int>(previous.y, current.y) + offset.y + size.y - 1;

            return SweptBounds{ minX, maxX, minY, maxY };
        }
    }

    void CollisionSystem::ProcessCollision(const std::vector<std::shared_ptr<Actor>> actors)
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

    bool CollisionSystem::IsCollide(const std::shared_ptr<Actor>& lhs, const std::shared_ptr<Actor>& rhs)
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

        const std::shared_ptr<BoxComponent>& lhsBox = lhs->GetComponent<BoxComponent>();
        const std::shared_ptr<BoxComponent>& rhsBox = rhs->GetComponent<BoxComponent>();

        if (!lhsBox || !rhsBox)
        {
            return false;
        }

        // 박스의 한 변 길이가 0 이하면 충돌하지 않는 Box
        const Vector2 lhsSize = lhsBox->GetSize();
        const Vector2 rhsSize = rhsBox->GetSize();

        if (lhsSize.x <= 0 || lhsSize.y <= 0 || 
            rhsSize.x <= 0 || rhsSize.y <= 0)
        {
            return false;
        }

        const SweptBounds lhsBounds = CalculateSweptBounds(*lhs, *lhsBox);
        const SweptBounds rhsBounds = CalculateSweptBounds(*rhs, *rhsBox);

        if (lhsBounds.maxX < rhsBounds.minX || 
            rhsBounds.maxX < lhsBounds.minX)
        {
            return false;
        }

        if (lhsBounds.maxY < rhsBounds.minY || 
            rhsBounds.maxY < lhsBounds.minY)
        {
            return false;
        }

        // 충돌 Yes
        return true;
    }

}
