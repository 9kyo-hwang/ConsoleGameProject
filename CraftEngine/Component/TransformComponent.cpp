#include "pch.h"
#include "TransformComponent.h"

namespace Craft
{
    TransformComponent::TransformComponent(const Vector2& localPosition)
        : localPosition(localPosition)
        , previousWorldPosition(localPosition)
    {

    }

    Vector2 TransformComponent::GetWorldPosition() const
    {
        if (auto parent = GetParent())
        {
            // ex) 3, 0 + 1, 2 -> 4, 2
            return parent->GetWorldPosition() + localPosition;
        }

        return localPosition;
    }

    void TransformComponent::SetWorldPosition(const Vector2& position)
    {
        if (auto parent = GetParent())
        {
            localPosition = position - parent->GetWorldPosition();
            return;
        }

        localPosition = position;
    }

    void TransformComponent::SavePreviousWorldPosition()
    {
        previousWorldPosition = GetWorldPosition();
    }

    void TransformComponent::AttachTo(const std::shared_ptr<TransformComponent>& newParent, bool keepWorldPosition)
    {
        if (!newParent)
        {
            return;
        }

        auto ancestor = newParent;
        while (ancestor && ancestor.get() != this)
        {
            ancestor = ancestor->GetParent();
        }

        if (ancestor.get() == this)
        {
            assert(false && "Transform hierarchy cycle detected");
            return;
        }

        const Vector2 previousWorld = GetWorldPosition();
        const Vector2 previousLocal = GetLocalPosition();

        // 기존 로컬 좌표를 새 부모 기준 오프셋으로 유지해야 하므로, 무조건 worldPosition 유지 안함
        DetachFromParent(false);

        parent = newParent;
        newParent->children.emplace_back(weak_from_this());

        keepWorldPosition
            ? SetWorldPosition(previousWorld)
            : SetLocalPosition(previousLocal);  // 재연결 시 기존 로컬 오프셋 유지
    }

    void TransformComponent::DetachFromParent(bool keepWorldPosition)
    {
        const Vector2 previousWorld = GetWorldPosition();

        if (auto oldParent = GetParent())
        {
            auto& sibling = oldParent->children;
            for (auto it = sibling.begin(); it != sibling.end(); ++it)
            {
                // 가정: 부모의 children에는 내 정보가 1개만 들어있음
                if ((*it).lock().get() == this)
                {
                    sibling.erase(it);
                    break;
                }
            }
        }

        parent.reset();

        if (keepWorldPosition)
        {
            SetLocalPosition(previousWorld);
        }
    }

    // children 내부의 expired된 객체를 걸러서 복사본으로 넘겨주기 위함
    // 지금은 자식 수가 많지 않아 복사본으로 넘김, 추후 많아지면 GetChildCount + GetChild(index) 형태로 변경
    std::vector<std::shared_ptr<TransformComponent>> TransformComponent::GetChildren() const
    {
        std::vector<std::shared_ptr<TransformComponent>> result;
        for (const auto& child : children)
        {
            if (auto childTransform = child.lock())
            {
                result.emplace_back(childTransform);
            }
        }

        return result;
    }
}
