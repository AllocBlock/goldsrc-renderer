#include "ImguiFGD.h"
#include "NativeSystem.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <string>

CImguiFGD::CImguiFGD()
{
}

void CImguiFGD::open() { m_IsOpen = true; }
void CImguiFGD::close() { m_IsOpen = false; }

void CImguiFGD::draw()
{
    if (!m_IsOpen) return;

    ImGui::Begin(u8"FGD");
    if (ImGui::Button(u8"打开FGD文件"))
    {
        auto Result = NativeSystem::createOpenFileDialog("fgd");
        if (Result)
        {
            m_pIOFGD = make<CIOGoldSrcForgeGameData>();
            m_pIOFGD->read(Result.FilePath);
        }
    }
    if (m_pIOFGD)
    {
        if (ImGui::Button(u8"关闭FGD文件"))
        {
            m_pIOFGD = nullptr;
        }
        else
        {
            ImGui::Text((u8"FGD实体数量：" + std::to_string(m_pIOFGD->getEntityNum())).c_str());
            if (ImGui::TreeNode(u8"FGD"))
            {
                for (size_t i = 0; i < m_pIOFGD->getEntityNum(); ++i)
                {
                    auto Entity = m_pIOFGD->getEntity(i);
                    if (ImGui::TreeNode(Entity.Name.c_str()))
                    {
                        for (const auto& Info : Entity.KeyValueInfoSet)
                        {
                            ImGui::Text(Info.Name.c_str()); ImGui::SameLine();
                            ImGui::Text(Info.DisplayName.c_str()); ImGui::SameLine();
                            ImGui::Text(Info.Type.c_str()); ImGui::SameLine();
                            ImGui::Text(Info.Default.c_str());
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }
    }
    ImGui::End();
}