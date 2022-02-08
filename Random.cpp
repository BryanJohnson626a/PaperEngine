#include "Random.h"

namespace Engine
{
	RNG::RNG(int seed)
	{
        if (seed == 0)
            seed = int(std::time(0));

        rng.seed(seed);

        int_distribution = std::uniform_int_distribution<int>(0, RNG_MAX_INT);
        float_distribution = std::uniform_real_distribution<float>(0.f, 1.f);
	}
    float RNG::operator()(float min, float max)
    {
        return min + float_distribution(rng) * (max - min);
    }
    int RNG::operator()(int min, int max)
    {
        return min + int_distribution(rng) % (max - min);
    }
}
