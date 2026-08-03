#pragma once

#include <Core/Core.h>
#include <memory>

namespace Craft
{
    // like UObject
    class CRAFT_API CObject
    {
    public:
        virtual size_t GetClass() const = 0;
        virtual bool IsA(size_t id) const { return false; }

        template<typename T>
        bool IsA() const { return IsA(T::GetTypeId()); }

        template<typename To, typename From>
        std::shared_ptr<To> Cast(const std::shared_ptr<From>& src)
        {
            if (src && src->IsA(To::TypeId()))
            {
                return std::static_pointer_cast<To>(src);
            }

            return nullptr;
        }
    };
}

#define TYPE_DECLARATIONS(Type, ParentType) \
    using Super = ParentType;               \
protected:                                  \
    static size_t GetTypeId()               \
    {                                       \
        static int runtimeTypeId = 0;       \
        return reinterpret_cast<size_t>(&runtimeTypeId);    \
    }                                       \
public:                                     \
    static size_t StaticClass()             \
    {                                       \
        return Type::GetTypeId();           \
    }                                       \
    size_t GetClass() const override        \
    {                                       \
        return Type::GetTypeId();           \
    }                                       \
    bool IsA(size_t id) const override      \
    {                                       \
        return (id == GetTypeId()) ? true : ParentType::IsA(id);    \
    }