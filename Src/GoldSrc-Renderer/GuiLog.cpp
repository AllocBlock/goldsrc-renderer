#include "GuiLog.h"
#include "Gui.h"

#include <chrono>
#include <iomanip>

std::istream& operator >> (std::istream& vIn, CImguiLog& vGUILog)
{
    std::string Temp;
    vIn >> Temp;
    vGUILog.log(Temp);
    return vIn;
}

void CImguiLog::log(std::string vText)
{
    std::string Time = __getCurrentTime();
    m_Logs.emplace_back(Time + vText);
    m_HasNewLog = true;
}

void CImguiLog::draw()
{
    if (m_Open)
    {
        UI::beginWindow(u8"日志");
        for (size_t i = 0; i < m_Logs.size(); ++i)
        {
            UI::text(m_Logs[i], true);
        }
        if (m_HasNewLog)
        {
            UI::setScrollHereY(1.0f);
            m_HasNewLog = false;
        }
        UI::endWindow();
    }
}

std::string CImguiLog::__getCurrentTime()
{
    time_t Time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm LocalTime;
    localtime_s(&LocalTime, &Time);
    std::string Result = u8"[";
    Result += std::to_string(1900 + LocalTime.tm_year) + u8"年";
    Result += std::to_string(1 + LocalTime.tm_mon) + u8"月";
    Result += std::to_string(LocalTime.tm_mday) + u8"日 ";
    Result += std::to_string(LocalTime.tm_hour) + u8"时";
    Result += std::to_string(LocalTime.tm_min) + u8"分";
    Result += std::to_string(LocalTime.tm_sec) + u8"秒";
    Result += u8"] ";
    return Result;
}