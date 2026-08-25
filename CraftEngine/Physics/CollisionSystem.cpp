#include "pch.h"
#include "CollisionSystem.h"
#include <Actor/Actor.h>
#include <Component/BoxComponent.h>
#include <Math/Box2D.h>

namespace Craft
{
    namespace
    {
        // 이전/이후 위치를 통해 Swept 크기를 구한 뒤 최종적으로 사각형 영역 생성
        Box2D CalculateSweptBounds(const Actor& actor, const BoxComponent& col)
        {
            const Vector2 previous = actor.GetPreviousPosition();
            const Vector2 current = actor.GetWorldPosition();
            
            const Vector2 size = col.GetSize();
            const Vector2 offset = col.GetOffset();

            const Vector2 minPosition(std::min<int>(previous.x, current.x) + offset.x, std::min<int>(previous.y, current.y) + offset.y);
            const Vector2 maxPosition(std::max<int>(previous.x, current.x) + offset.x, std::max<int>(previous.y, current.y) + offset.y);
            const Vector2 sweptSize = maxPosition - minPosition + size; // -1은 안하는 건가?

            return Box2D{ minPosition, sweptSize };
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

    /*
    * AABB(Axis-Aligned Bounding Box) Collision
    * float로 계산한 걸 int 좌표로 표현하다보니,
    * 빠르게 이동하는 물체는 한 프레임에 몇 칸을 넘어갈 수 있음.
    * 이 경우 이동 경로 중간에 통과할 수 없는 물체(적 등)가 있어도
    * 무시하고 건너뛰어지는 경우가 종종 발생.
    * 따라서 이전 프레임 위치 정보를 이용, 이전 위치 ~ 현재 위치를 아우르는 긴 박스를 만들고
    * 이 박스와 충돌한 액터가 있는지 판정하는 식으로 보강
    */
    bool CollisionSystem::IsCollide(const std::shared_ptr<Actor>& lhs, const std::shared_ptr<Actor>& rhs)
    {
        if (!lhs || !lhs->IsActive() || !rhs || !rhs->IsActive())
        {
            return false;
        }

        const std::shared_ptr<BoxComponent>& lhsBox = lhs->GetComponent<BoxComponent>();
        const std::shared_ptr<BoxComponent>& rhsBox = rhs->GetComponent<BoxComponent>();

        if (!lhsBox || !rhsBox)
        {
            return false;
        }

        const Box2D lhsBounds = CalculateSweptBounds(*lhs, *lhsBox);
        const Box2D rhsBounds = CalculateSweptBounds(*rhs, *rhsBox);

        // BoxSize 검사도 내부적으로 IsValid를 통해 수행
        return lhsBounds.Overlaps(rhsBounds);
    }
}
