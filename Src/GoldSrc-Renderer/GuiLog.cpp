#include "GuiLog.h"
#include "InterfaceUI.h"

#include <chrono>
#include <iomanip>

std::istream& operator >> (std::istream& vIn, CGuiLog& vGUILog)
{
    std::string Temp;
    vIn >> Temp;
    vGUILog.log(Temp);
    return vIn;
}

void CGuiLog::log(std::string vText)
{
    std::string Time = __getCurrentTime();
    m_Logs.emplace_back(Time + vText);
    m_HasNewLog = true;
}

void CGuiLog::draw()
{
    if (m_Open)
    {
        UI::beginWindow(u8"��־");
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

std::string CGuiLog::__getCurrentTime()
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