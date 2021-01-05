#pragma once

#include <cfloat>
#include <random>
#include <stdint.h>

/*
	See https://stackoverflow.com/a/16224552/7157626
*/
class Random
{
  public:
    Random(uint32_t seed);
    Random();

    Random(const Random &other) = delete;
    Random &operator=(const Random &other) = delete;

    /*
		Random double in range [0, 1].
	*/
    double nextDouble();

    /*
		Random int in range [from, to).
	*/
    template <typename T, typename A = int>
    T nextInt(A from, A to)
    {
        double r = nextDouble();
        A maxValue = std::max(from, to - 1);

        return static_cast<T>(std::round(from + r * (maxValue - from)));
    }

    void setSeed(uint32_t seed);

    static Random &global();

  private:
    std::mt19937 randomEngine;

    std::uniform_real_distribution<double> distribution;

    void initialize(uint32_t seed);
};