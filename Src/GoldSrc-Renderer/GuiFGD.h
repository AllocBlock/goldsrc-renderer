#pragma once
#include "Pointer.h"
#include "IOGoldSrcForgeGameData.h"
#include "GuiSelectFile.h"

#include <filesystem>

class CImguiFGD
{
public:
    CImguiFGD();
    void draw();
    void open();
    void close();

private:
    void __requestFGDFile();

    CImguiSelectFile m_FileSelection;
    bool m_IsOpen = true;
    ptr<CIOGoldSrcForgeGameData> m_pIOFGD = nullptr;
    std::future<std::filesystem::path> m_Future;
};

