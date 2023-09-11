#pragma once
#include "Common.h"
#include <string>
#include <filesystem>
#include <optional>

class CGuiLoading
{
public:
    void update(const Common::SLoadingInfo& vLoadingInfo);
    void end();

    void renderUI();

private:
    bool m_CloseAtNextFrame = false;
   
    Common::SLoadingInfo m_LoadingInfo;
};
