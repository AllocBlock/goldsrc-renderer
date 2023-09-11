#pragma once
#include "Common.h"
#include "GuiWindow.h"
#include <vector>
#include <string>

enum class ELogLevel
{
    VERBOSE,
    DEBUG,
    WARNING,
    ERROR
};

class CGuiLog : public CGuiWindow
{
public:
    void log(const std::string& vText, ELogLevel vLevel = ELogLevel::VERBOSE);
    
protected:
    virtual std::string _getWindowNameV() override { return u8"»’÷æ"; }
    virtual void _renderUIV() override;

private:
    std::string __getCurrentTimeString() const;
    
    bool m_JumpToNewest = false;
    std::vector<std::string> m_Logs;
    std::vector<ELogLevel> m_Levels;
};

