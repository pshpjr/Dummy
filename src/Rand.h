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

        //  중앙값 median, 평균 mean인 로그정규분포에서 하나 뽑는다. mean이 median 이하면 median을 돌려준다.
        static double LogNormal(double median, double mean)
        {
            if (mean <= median)
                return median;

            std::lognormal_distribution<double> dis(std::log(median), std::sqrt(2.0 * std::log(mean / median)));
            return dis(gen);
        }

        //  probability(0~1) 확률로 true
        static bool Chance(double probability)
        {
            std::uniform_real_distribution<double> dis(0.0, 1.0);
            return dis(gen) < probability;
        }
    };

}
