#pragma once
#include "Pointer.h"
#include "IOGoldSrcForgeGameData.h"

#include <filesystem>

class CGuiFGD
{
public:
    CGuiFGD();
    void draw();
    void open();
    void close();

private:
    void __requestFGDFile();

    bool m_IsOpen = true;
    sptr<CIOGoldSrcForgeGameData> m_pIOFGD = nullptr;
};

