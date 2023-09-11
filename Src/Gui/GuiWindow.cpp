#include "GuiWindow.h"
#include "InterfaceGui.h"

void CGuiWindow::renderUI()
{
    if (m_Show)
    {
        UI::beginWindow(_getWindowNameV());
        _renderUIV();
        UI::endWindow();
    }
}
