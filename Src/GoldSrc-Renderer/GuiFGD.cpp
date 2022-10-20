#include "GuiFGD.h"
#include "InterfaceUI.h"
#include "NativeSystem.h"

#include <string>

CGuiFGD::CGuiFGD()
{
}

void CGuiFGD::open() { m_IsOpen = true; }
void CGuiFGD::close() { m_IsOpen = false; }

void CGuiFGD::draw()
{
    if (!m_IsOpen) return;

    UI::beginWindow(u8"FGD");
    if (UI::button(u8"打开FGD文件"))
    {
        auto Result = Gui::createOpenFileDialog("fgd");
        if (Result)
        {
            m_pIOFGD = make<CIOGoldSrcForgeGameData>();
            m_pIOFGD->read(Result.FilePath);
        }
    }
    if (m_pIOFGD)
    {
        if (UI::button(u8"关闭FGD文件"))
        {
            m_pIOFGD = nullptr;
        }
        else
        {
            UI::text((u8"FGD实体数量：" + std::to_string(m_pIOFGD->getEntityNum())));
            if (UI::treeNode(u8"FGD"))
            {
                for (size_t i = 0; i < m_pIOFGD->getEntityNum(); ++i)
                {
                    auto Entity = m_pIOFGD->getEntity(i);
                    if (UI::treeNode(Entity.Name))
                    {
                        for (const auto& Info : Entity.KeyValueInfoSet)
                        {
                            UI::text(Info.Name); UI::sameLine();
                            UI::text(Info.DisplayName); UI::sameLine();
                            UI::text(Info.Type); UI::sameLine();
                            UI::text(Info.Default);
                        }
                        UI::treePop();
                    }
                }
                UI::treePop();
            }
        }
    }
    UI::endWindow();
}