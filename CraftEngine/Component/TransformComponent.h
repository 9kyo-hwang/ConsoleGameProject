#pragma once
#include <Component/ActorComponent.h>
#include <Math/Vector2.h>

namespace Craft
{
    class TransformComponent : public ActorComponent
    {
        TYPE_DECLARATIONS(TransformComponent, ActorComponent)

    public:
        TransformComponent(const Vector2& localPosition = Vector2::Zero);
        ~TransformComponent() override = default;

        // TODO: 부모 계층 붙으면 위치 Getter/Setter 로직 수정
        inline Vector2 GetLocalPosition() const { return localPosition; }
        inline void SetLocalPosition(const Vector2& position) { localPosition = position; } 
        Vector2 GetWorldPosition() const;   
        void SetWorldPosition(const Vector2& position);

        inline Vector2 GetPreviousWorldPosition() const { return previousWorldPosition; }

        void SavePreviousWorldPosition();
        
    protected:
        // Actor가 들고 있던 위치 정보를 이관
        Vector2 localPosition;  // 부모 기준으로 얼마나 떨어져있는가
        Vector2 worldPosition;  // 월드 좌표 기준으로 어디에 위치하는가(일단은 localPosition으로 다 사용)
        Vector2 previousWorldPosition;
    };
}

