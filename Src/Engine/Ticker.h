#pragma once
#include <chrono>

template <typename T = float>
class CTicker
{
public:
    CTicker()
    {
        start();
    }

    void start()
    {
        static_assert(std::is_floating_point_v<T>, "Type T should be floating point");
        m_Last = std::chrono::system_clock::now();
    }

    T update()
    {
        auto Current = std::chrono::system_clock::now();
        auto Duration = std::chrono::duration_cast<std::chrono::nanoseconds>(Current - m_Last);
        T DeltaSecond = T(Duration.count()) * 1e-9f;
        m_Last = Current;
        return DeltaSecond;
    }
private:
    std::chrono::time_point<std::chrono::system_clock> m_Last;
};
