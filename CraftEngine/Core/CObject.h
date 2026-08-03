#pragma once

#include <Core/Core.h>
#include <memory>

using TypeId = std::uintptr_t;

namespace Craft
{
    // like UObject
    class CRAFT_API CObject
    {
    public:
        virtual TypeId GetClass() const = 0;
        virtual bool IsA(TypeId Id) const { return false; }

        template<typename T>
        bool IsA() const { return IsA(T::StaticClass()); }

        template<typename To, typename From>
        std::shared_ptr<To> Cast(const std::shared_ptr<From>& Src)
        {
            if (Src && Src->IsA(To::StaticClass()))
            {
                return std::static_pointer_cast<To>(Src);
            }

            return nullptr;
        }
    };
}

#define TYPE_DECLARATIONS(Type, ParentType) \
    using ThisClass = Type;                 \
    using Super = ParentType;               \
protected:                                  \
    static TypeId GetTypeId()               \
    {                                       \
        static int RuntimeTypeId = 0;       \
        return reinterpret_cast<TypeId>(&RuntimeTypeId);    \
    }                                       \
public:                                     \
    using ParentType::IsA;                  \
    static TypeId StaticClass()             \
    {                                       \
        return Type::GetTypeId();           \
    }                                       \
    TypeId GetClass() const override        \
    {                                       \
        return Type::StaticClass();         \
    }                                       \
    bool IsA(TypeId Id) const override      \
    {                                       \
        return Id == StaticClass() || Super::IsA(Id);   \
    }