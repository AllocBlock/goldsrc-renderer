#include "GUILog.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <chrono>
#include <iomanip>

std::istream& operator >> (std::istream& vIn, CGUILog& vGUILog)
{
    std::string Temp;
    vIn >> Temp;
    vGUILog.log(Temp);
    return vIn;
}

void CGUILog::log(std::string vText)
{
    std::string Time = __getCurrentTime();
    m_Logs.emplace_back(Time + vText);
}

void CGUILog::draw()
{
    if (m_Open)
    {
        ImGui::Begin(u8"��־");
        for (const std::string& Log : m_Logs)
        {
            ImGui::TextWrapped(Log.c_str());
        }
        ImGui::End();
    }
}

std::string CGUILog::__getCurrentTime()
{
    time_t Time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm LocalTime;
    localtime_s(&LocalTime, &Time);
    std::string Result = u8"[";
    Result += std::to_string(1900 + LocalTime.tm_year) + u8"��";
    Result += std::to_string(1 + LocalTime.tm_mon) + u8"��";
    Result += std::to_string(LocalTime.tm_mday) + u8"�� ";
    Result += std::to_string(LocalTime.tm_hour) + u8"ʱ";
    Result += std::to_string(LocalTime.tm_min) + u8"��";
    Result += std::to_string(LocalTime.tm_sec) + u8"��";
    Result += u8"] ";
    return Result;
}