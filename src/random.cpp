#include "random.h"

Random globalRandom;

Random::Random()
{
    std::random_device device;
    uint32_t seed = device();
    initialize(seed);
}

Random::Random(uint32_t seed)
{
    initialize(seed);
}

void Random::initialize(uint32_t seed)
{
    this->distribution = std::uniform_real_distribution<double>(0, std::nextafter(1, DBL_MAX));
    std::mt19937 mt(seed);
    randomEngine = mt;
}

void Random::setSeed(uint32_t seed)
{
    initialize(seed);
}

double Random::nextDouble()
{
    return distribution(randomEngine);
}

Random &Random::global()
{
    return globalRandom;
}