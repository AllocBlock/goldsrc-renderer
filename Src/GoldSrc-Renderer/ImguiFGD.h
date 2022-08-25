#pragma once
#include "Pointer.h"
#include "IOGoldSrcForgeGameData.h"

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

    bool m_IsOpen = true;
    ptr<CIOGoldSrcForgeGameData> m_pIOFGD = nullptr;
};

