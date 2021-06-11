#pragma once
#include <chrono>
#include <vector>

class CImguiFrameRate
{
public:
    CImguiFrameRate();
    void draw();

private:
    std::chrono::milliseconds m_LastTimeStamp = std::chrono::milliseconds(0);
    std::chrono::milliseconds m_LastAverageTimeStamp = std::chrono::milliseconds(0);
    size_t m_SavedNum = 300;
    std::vector<float> m_SavedFrameRates;
    size_t m_AverageFrameRateInCountNum = 10;
    float m_AverageFrameRateUpdateInterval = 1.0f;
};

