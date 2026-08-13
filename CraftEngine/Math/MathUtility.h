#pragma once

#include <random>
#include <cassert>

namespace Craft
{
    struct FMath
    {
        static void SeedRandomDevice()
        {
            static std::random_device device;
            Engine().seed(device());
        }

        static void SetRandomSeed(uint32_t newSeed)
        {
            Engine().seed(newSeed);
        }

        // Unreal FMath::RandRange(int32, int32)처럼 양 끝값 포함.
        static int RandRange(int min, int max)
        {
            assert(min <= max);
            return std::uniform_int_distribution<int>(min, max)(Engine());
        }

        // C++ 표준 분포의 의미: [min, max)
        static float RandRange(float min, float max)
        {
            assert(min <= max);
            return std::uniform_real_distribution<float>(min, max)(Engine());
        }

    private:
        static std::mt19937& Engine()
        {
            static std::mt19937 engine;
            return engine;
        }
    };
}