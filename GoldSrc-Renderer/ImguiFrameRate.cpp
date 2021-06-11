#include "ImguiFrameRate.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <string>

CImguiFrameRate::CImguiFrameRate()
{
    m_LastTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
}

void CImguiFrameRate::draw()
{
    std::chrono::milliseconds CurTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
    float DeltaTimeSecond = static_cast<float>((CurTimeStamp - m_LastTimeStamp).count()) / 1000.0f;
    float RealtimeFPS = 1.0f / DeltaTimeSecond;
    m_SavedFrameRates.push_back(RealtimeFPS);
    if (m_SavedFrameRates.size() > m_SavedNum)
    {
        float PoppedFrameRate = m_SavedFrameRates.front();
        m_SavedFrameRates.erase(m_SavedFrameRates.begin(), m_SavedFrameRates.begin() + 1);
    }
    
    ImGui::Begin(u8"帧率", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize);
    ImGui::Text((u8"实时FPS: " + std::to_string(RealtimeFPS)).c_str());
    float AverageDeltaTimeSecond = static_cast<float>((CurTimeStamp - m_LastAverageTimeStamp).count()) / 1000.0;
    static float DisplayedAverageFrameRate = 0.0;
    if (AverageDeltaTimeSecond >= m_AverageFrameRateUpdateInterval)
    {
        m_LastAverageTimeStamp = CurTimeStamp;
        DisplayedAverageFrameRate = 0.0f;
        for (size_t i = std::max<size_t>(0, m_SavedFrameRates.size() - m_AverageFrameRateInCountNum); i < m_SavedFrameRates.size(); ++i)
            DisplayedAverageFrameRate += m_SavedFrameRates[i];
        DisplayedAverageFrameRate /= std::min<size_t>(m_AverageFrameRateInCountNum, m_SavedFrameRates.size());
    }
    ImGui::Text((u8"平均FPS: " + std::to_string(DisplayedAverageFrameRate)).c_str());

    ImGui::PlotLines(u8"帧率图", m_SavedFrameRates.data(), m_SavedFrameRates.size());
    ImGui::End();
    m_LastTimeStamp = CurTimeStamp;
}
