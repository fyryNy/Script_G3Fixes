#ifndef RANDOM_INCLUDED
#define RANDOM_INCLUDED

#include <algorithm>
#include <random>

class Randomizer {
private:
	std::mt19937 gen;

	void Seed() {
		std::random_device rdev;
		std::mt19937::result_type data[std::mt19937::state_size];
		std::generate_n(data, std::mt19937::state_size, std::ref(rdev));
		std::seed_seq prng_seed(data, data + std::mt19937::state_size);
		gen.seed(prng_seed);
	}

public:
	Randomizer() {
		Seed();
	};

	GEInt Random(GEInt min, GEInt max) {
		std::uniform_int_distribution<unsigned> distrib(min, max);
		return distrib(gen);
	}
};
Randomizer randomizer;

#endif