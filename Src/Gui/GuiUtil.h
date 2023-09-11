#pragma once
#include "GuiAlert.h"
#include "GuiLoading.h"
#include "GuiLog.h"

struct SGuiUtil
{
public:
    void renderUI()
    {
        // loading
        Loading.renderUI();

        // alert
        Alert.renderUI();

        // log
        Log.renderUI();
    }
    
    CGuiAlert Alert;
    CGuiLoading Loading;
    CGuiLog Log;
};