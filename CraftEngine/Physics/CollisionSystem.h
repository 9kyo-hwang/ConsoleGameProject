#pragma once

#include <memory>
#include <vector>

namespace Craft
{
    class Actor;    // 액터 간 충돌 판정이 주
    class CollisionSystem
    {
        // 충돌이 발생한 두 액터 A, B를 기억, 이벤트 한 번에 발행
        using CollisionPair = std::pair<std::shared_ptr<Actor>, std::shared_ptr<Actor>>;

    public:
        CollisionSystem() = default;
        ~CollisionSystem() = default;

        // 액터 목록을 순회해 충돌 판정
        void ProcessCollision(const std::vector<std::shared_ptr<Actor>> actors);

    private:
        bool IsCollide(const std::shared_ptr<Actor>& lhs, const std::shared_ptr<Actor>& rhs);
    };
}
