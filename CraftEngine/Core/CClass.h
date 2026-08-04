#pragma once

#include <Core/Core.h>
#include <memory>
#include <stdexcept>
#include <string_View>
#include <type_traits>

namespace Craft
{
    class CObject;

    // like UClass
    struct CRAFT_API CClass
    {
        using Factory = std::shared_ptr<CObject>(*)();

        std::string_view Name;
        const CClass* SuperClass = nullptr;
        Factory CreateInstance = nullptr;

        bool IsChildOf(const CClass& Base) const
        {
            for (auto Current = this; Current != nullptr; Current = Current->SuperClass)
            {
                if (Current == &Base)
                {
                    return true;
                }
            }

            return false;
        }

        std::shared_ptr<CObject> NewObject() const
        {
            return CreateInstance ? CreateInstance() : nullptr;
        }
    };
}