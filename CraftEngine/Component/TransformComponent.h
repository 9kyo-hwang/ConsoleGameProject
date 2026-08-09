#pragma once
#include <Component/ActorComponent.h>
#include <Math/Vector2.h>

namespace Craft
{
    // AttachTo에서 자기자신을 부모의 children에 넣게 하도록(weak_from_this())
    class CRAFT_API TransformComponent : public ActorComponent, public std::enable_shared_from_this<TransformComponent>
    {
        TYPE_DECLARATIONS(TransformComponent, ActorComponent)

    public:
        TransformComponent(const Vector2& localPosition = Vector2::Zero);
        ~TransformComponent() override = default;

        inline Vector2 GetLocalPosition() const { return localPosition; }
        inline void SetLocalPosition(const Vector2& position) { localPosition = position; } 

        Vector2 GetWorldPosition() const;   
        void SetWorldPosition(const Vector2& position);

        inline Vector2 GetPreviousWorldPosition() const { return previousWorldPosition; }
        void SavePreviousWorldPosition();

        // 기존 Actor가 담당하던 책임을 Transform으로 이관
        void AttachTo(const std::shared_ptr<TransformComponent>& newParent, bool keepWorldPosition = true);
        void DetachFromParent(bool keepWorldPosition = true);

        inline std::shared_ptr<TransformComponent> GetParent() const { return parent.lock(); }
        std::vector<std::shared_ptr<TransformComponent>> GetChildren() const;
        
    protected:
        // Actor가 들고 있던 위치 정보를 이관
        Vector2 localPosition;  // 부모 기준으로 얼마나 떨어져있는가
        //Vector2 worldPosition;  // 월드 좌표 기준으로 어디에 위치하는가
        Vector2 previousWorldPosition;

        std::weak_ptr<TransformComponent> parent;
        std::vector<std::weak_ptr<TransformComponent>> children;
    };
}

