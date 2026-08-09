#pragma once
#include <Component/ActorComponent.h>
#include <Math/Vector2.h>

namespace Craft
{
    class CRAFT_API TransformComponent : public ActorComponent
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

        inline std::shared_ptr<TransformComponent> GetParent() const { return parent.lock(); }
        inline void SetParent(std::weak_ptr<TransformComponent> newParent) { parent = newParent; }
        
    protected:
        // Actor가 들고 있던 위치 정보를 이관
        Vector2 localPosition;  // 부모 기준으로 얼마나 떨어져있는가
        Vector2 worldPosition;  // 월드 좌표 기준으로 어디에 위치하는가(일단은 localPosition으로 다 사용)
        Vector2 previousWorldPosition;

        std::weak_ptr<TransformComponent> parent;   // 부모를 약참조로 가리키자.
    };
}

