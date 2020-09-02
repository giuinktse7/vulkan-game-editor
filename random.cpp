#include "random.h"

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

double Random::nextDouble()
{
  return distribution(randomEngine);
}
