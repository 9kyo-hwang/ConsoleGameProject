#pragma once
#include <Core/CObject.h>
#include <Core/Core.h>

namespace Craft
{
    class Actor;    // ActorComponent의 Owner
    class CRAFT_API ActorComponent : public CObject
    {
        TYPE_DECLARATIONS(ActorComponent, CObject)

    public:
        ActorComponent() = default;
        ~ActorComponent() override = default;

        virtual void BeginPlay() { hasBeganPlay = true; }
        virtual void Tick(float deltaTime) {}
        virtual void Draw() {}
        virtual void OnCollision(const std::shared_ptr<Actor>& other) {}
        virtual void EndPlay() {}

        inline bool HasBeganPlay() const { return hasBeganPlay; }
        inline std::shared_ptr<Actor> GetOwner() const { return owner.lock(); }
        inline void SetOwner(std::weak_ptr<Actor> newOwner) { owner = newOwner; }

    protected:
        bool hasBeganPlay = false;
        std::weak_ptr<Actor> owner;
    };
}
