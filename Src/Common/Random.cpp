#include "Random.h"
#include <random>

std::uniform_real_distribution<float> gUniform(0, 1);
std::default_random_engine gEngine;

float Random::GenerateFloat(float vMin, float vMax)
{
    return gUniform(gEngine) * (vMax - vMin) + vMin;
}

int Random::GenerateIntegar(int vMin, int vMax)
{
    return int(Random::GenerateFloat(vMin, vMax));
}