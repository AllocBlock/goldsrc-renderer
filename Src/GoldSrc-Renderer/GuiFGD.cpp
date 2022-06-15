#include "GuiFGD.h"
#include "Gui.h"

#include <string>

CImguiFGD::CImguiFGD()
{
}

void CImguiFGD::open() { m_IsOpen = true; }
void CImguiFGD::close() { m_IsOpen = false; }

void CImguiFGD::draw()
{
    // 读取FGD
    if (m_Future.valid())
    {
        m_FileSelection.draw();
        if (m_Future._Is_ready())
        {
            auto Path = m_Future.get();
            if (!Path.empty())
            {
                m_pIOFGD = make<CIOGoldSrcForgeGameData>();
                m_pIOFGD->read(Path);
            }
        }
    }

    if (!m_IsOpen) return;

    UI::beginWindow(u8"FGD");
    if (UI::button(u8"打开FGD文件"))
    {
        __requestFGDFile();
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

void CImguiFGD::__requestFGDFile()
{
    m_FileSelection.setTitle(u8"FGD");
    m_FileSelection.setFilters({ ".fgd" });
    std::promise<std::filesystem::path> Promise;
    m_Future = Promise.get_future();
    m_FileSelection.start(std::move(Promise));
}
