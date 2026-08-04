#pragma once

#include <Core/Core.h>
#include <Core/CClass.h>
#include <memory>

namespace Craft
{
    // like UObject
    class CRAFT_API CObject
    {
    public:
        virtual ~CObject() = default;

        virtual const CClass& GetClass() const 
        { 
            return StaticClass(); 
        }

        bool IsA(const CClass& SomeBase) const 
        { 
            return GetClass().IsChildOf(SomeBase);
        }

        template<typename T>
        bool IsA() const 
        { 
            return IsA(T::StaticClass()); 
        }

        static const CClass& StaticClass()
        {
            static const CClass Class
            {
                "CObject", nullptr, [] {return std::make_shared<CObject>();}
            };
            return Class;
        }
    };

    template<typename To, typename From>
    std::shared_ptr<To> Cast(const std::shared_ptr<From>& Src)
    {
        static_assert(std::is_base_of_v<CObject, To>);
        static_assert(std::is_base_of_v<CObject, From>);

        if (Src && Src->IsA(To::StaticClass()))
        {
            return std::static_pointer_cast<To>(Src);
        }

        return nullptr;
    }
}


#define TYPE_DECLARATIONS(Type, Parent)                     \
public:                                                     \
    using ThisClass = Type;                                 \
    using Super = Parent;                                   \
                                                            \
    static const Craft::CClass& StaticClass()               \
    {                                                       \
        static const Craft::CClass Class                    \
        {                                                   \
            #Type, &Parent::StaticClass(),                  \
            []() -> std::shared_ptr<Craft::CObject>         \
            {                                               \
                if constexpr (std::is_abstract_v<Type> ||   \
                    !std::is_default_constructible_v<Type>) \
                {                                           \
                    return nullptr;                         \
                }                                           \
                else return std::make_shared<Type>();       \
            }                                               \
        };                                                  \
        return Class;                                       \
    }                                                       \
    const Craft::CClass& GetClass() const override           \
    {                                                       \
        return StaticClass();                               \
    }