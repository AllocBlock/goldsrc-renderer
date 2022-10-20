#include "GuiFrameRate.h"
#include "InterfaceUI.h"

#include <string>

CGuiFrameRate::CGuiFrameRate()
{
    m_LastTimeStamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
}

void CGuiFrameRate::draw()
{
    std::chrono::microseconds CurTimeStamp = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch());
    double DeltaTimeSecond = static_cast<double>((CurTimeStamp - m_LastTimeStamp).count()) / 1e6;
    double RealtimeFPS = 1.0f / DeltaTimeSecond;
    m_SavedFrameRates.push_back(RealtimeFPS);
    if (m_SavedFrameRates.size() > m_SavedNum)
    {
        double PoppedFrameRate = m_SavedFrameRates.front();
        m_SavedFrameRates.erase(m_SavedFrameRates.begin(), m_SavedFrameRates.begin() + 1);
    }
    
    UI::beginWindow(u8"֡��", nullptr, UI::EWindowFlag::ALWAYS_AUTO_RESIZE | UI::EWindowFlag::NO_RESIZE);
    UI::text(u8"ʵʱFPS: " + std::to_string(RealtimeFPS));
    double AverageDeltaTimeSecond = static_cast<float>((CurTimeStamp - m_LastAverageTimeStamp).count()) / 1e6;
    static double DisplayedAverageFrameRate = 0.0;
    if (AverageDeltaTimeSecond >= m_AverageFrameRateUpdateInterval)
    {
        m_LastAverageTimeStamp = CurTimeStamp;
        DisplayedAverageFrameRate = 0.0f;
        for (size_t i = std::max<size_t>(0, m_SavedFrameRates.size() - m_AverageFrameRateInCountNum); i < m_SavedFrameRates.size(); ++i)
            DisplayedAverageFrameRate += m_SavedFrameRates[i];
        DisplayedAverageFrameRate /= std::min<size_t>(m_AverageFrameRateInCountNum, m_SavedFrameRates.size());
    }
    UI::text(u8"ƽ��FPS: " + std::to_string(DisplayedAverageFrameRate));

    UI::plotLines(u8"֡��ͼ", m_SavedFrameRates);
    UI::endWindow();
    m_LastTimeStamp = CurTimeStamp;
}
