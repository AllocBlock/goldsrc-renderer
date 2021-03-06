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

void CGUILog::log(std::filesystem::path vText)
{
    std::string Time = __getCurrentTime();
    m_Logs.emplace_back(Time + vText.string());
    m_HasNewLog = true;
}

void CGUILog::draw()
{
    if (m_Open)
    {
        ImGui::Begin(u8"日志");
        for (const std::filesystem::path& Log : m_Logs)
        //for (size_t i = 0; i < m_Logs.size(); ++i)
        {
            //const std::filesystem::path& Log = m_Logs[i];
            /*const char* pStr = Log.data();
            const char* pEnd = pStr + Log.length();
            ImGui::PushTextWrapPos();
            ImGui::TextUnformatted(pStr, pEnd);
            ImGui::PopTextWrapPos();*/
            ImGui::TextWrapped(Log.u8string().c_str());
        }
        if (m_HasNewLog)
        {
            ImGui::SetScrollHereY(1.0f);
            m_HasNewLog = false;
        }
        ImGui::End();
    }
}

std::string CGUILog::__getCurrentTime()
{
    time_t Time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm LocalTime;
    localtime_s(&LocalTime, &Time);
    std::string Result = "[";
    Result += std::to_string(1900 + LocalTime.tm_year) + "年";
    Result += std::to_string(1 + LocalTime.tm_mon) + "月";
    Result += std::to_string(LocalTime.tm_mday) + "日 ";
    Result += std::to_string(LocalTime.tm_hour) + "时";
    Result += std::to_string(LocalTime.tm_min) + "分";
    Result += std::to_string(LocalTime.tm_sec) + "秒";
    Result += "] ";
    return Result;
}