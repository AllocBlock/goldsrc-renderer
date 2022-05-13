#pragma once
#include <chrono>
#include <vector>

class CImguiFrameRate
{
public:
    CImguiFrameRate();
    void draw();

private:
    std::chrono::microseconds m_LastTimeStamp = std::chrono::microseconds(0);
    std::chrono::microseconds m_LastAverageTimeStamp = std::chrono::microseconds(0);
    size_t m_SavedNum = 300;
    std::vector<float> m_SavedFrameRates;
    size_t m_AverageFrameRateInCountNum = 10;
    float m_AverageFrameRateUpdateInterval = 1.0f;
};

