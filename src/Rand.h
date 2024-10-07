#pragma once

#pragma once
#include <random>

namespace psh
{
    struct RandomUtil
    {
        inline static thread_local std::mt19937 gen = std::mt19937(0);

        static void SRand(int seed) { gen =  std::mt19937(seed);}

        static int Rand(int min, int max)
        {
            std::uniform_int_distribution<int> dis(min, max);
            return dis(gen);
        }
    };

}
