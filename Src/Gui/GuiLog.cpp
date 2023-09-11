#include "GuiLog.h"
#include "InterfaceGui.h"

#include <chrono>
#include <map>

const std::map<ELogLevel, glm::vec4> gLogLevelColorMap = {
    {ELogLevel::VERBOSE, glm::vec4(1.0, 1.0, 1.0, 1.0) },
    {ELogLevel::DEBUG,   glm::vec4(1.0, 1.0, 1.0, 1.0) },
    {ELogLevel::WARNING, glm::vec4(1.0, 0.6, 0.0, 1.0) },
    {ELogLevel::ERROR,   glm::vec4(1.0, 0.3, 0.3, 1.0) },
};

void CGuiLog::log(const std::string& vText, ELogLevel vLevel)
{
    std::string Time = __getCurrentTimeString();
    m_Logs.emplace_back(Time + vText);
    m_Levels.emplace_back(vLevel);
    m_JumpToNewest = true;
}

void CGuiLog::_renderUIV()
{
    for (size_t i = 0; i < m_Logs.size(); ++i)
    {
        ELogLevel Level = m_Levels[i];
        UI::pushStyleColor(UI::EStyleColorTarget::TEXT, gLogLevelColorMap.at(Level));
        UI::text(m_Logs[i], true);
        UI::popStyleColor();
    }
    if (m_JumpToNewest)
    {
        UI::setScrollHereY(1.0f);
        m_JumpToNewest = false;
    }
}

std::string CGuiLog::__getCurrentTimeString() const
{
    time_t Time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    tm LocalTime;
    localtime_s(&LocalTime, &Time);
    std::string Result = u8"[";
    Result += std::to_string(1900 + LocalTime.tm_year) + u8"-";
    Result += std::to_string(1 + LocalTime.tm_mon) + u8"-";
    Result += std::to_string(LocalTime.tm_mday) + u8" ";
    Result += std::to_string(LocalTime.tm_hour) + u8":";
    Result += std::to_string(LocalTime.tm_min) + u8":";
    Result += std::to_string(LocalTime.tm_sec);
    Result += u8"] ";
    return Result;
}
