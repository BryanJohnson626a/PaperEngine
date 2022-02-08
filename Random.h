#pragma once
#include "Core.h"
#include <random>
#include <limits>

namespace Engine
{
	class RNG
	{
	public:
		const int RNG_MAX_INT = std::numeric_limits<int>::max();

		RNG(int seed = 0);
		float operator()(float min, float max);
		int operator()(int min, int max);
	private:
		std::random_device dev;
		std::mt19937 rng;
		std::uniform_int_distribution<int> int_distribution;
		std::uniform_real_distribution<float> float_distribution;
	};
}
