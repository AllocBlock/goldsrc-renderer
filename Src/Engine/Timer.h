#pragma once
#include <chrono>

class CTimer
{
public:
    CTimer()
    {
        start();
    }

    void start()
    {
        m_LastTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    }

    float tick() // return delta seconds
    {
        std::chrono::milliseconds CurrentTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
        float DeltaTime = static_cast<float>((CurrentTimeStamp - m_LastTimeStamp).count()) / 1000.0f;
        m_LastTimeStamp = CurrentTimeStamp;
        return DeltaTime;
    }

private:
    std::chrono::milliseconds m_LastTimeStamp;
};