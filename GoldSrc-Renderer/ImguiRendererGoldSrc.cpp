#include "ImguiRendererGoldSrc.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void CImguiRendererGoldSrc::_setTargetV(std::shared_ptr<CRendererScene> vRenderer)
{
    m_pRenderer = std::static_pointer_cast<CRendererSceneGoldSrc>(vRenderer);
}

void CImguiRendererGoldSrc::_drawV()
{
    if (!m_pRenderer) return;
    if (ImGui::CollapsingHeader(u8"��Ⱦ������", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool EnableBSP = m_pRenderer->getBSPState();
        ImGui::Checkbox(u8"ʹ��BSP����Ⱦ��֧��ʵ����Ⱦģʽ��", &EnableBSP);
        m_pRenderer->setBSPState(EnableBSP);

        bool SkyRendering = m_pRenderer->getSkyState();
        ImGui::Checkbox(u8"���������Ⱦ", &SkyRendering);
        m_pRenderer->setSkyState(SkyRendering);

        bool Culling = m_pRenderer->getCullingState();
        ImGui::Checkbox(u8"�����޳�", &Culling);
        m_pRenderer->setCullingState(Culling);
        Culling = m_pRenderer->getCullingState();

        if (Culling)
        {
            ImGui::Indent(20.0f);
            ImGui::BeginGroup();
            bool FrustumCulling = m_pRenderer->getFrustumCullingState();
            ImGui::Spacing();
            ImGui::Checkbox(u8"CPU��׶�޳�", &FrustumCulling);
            m_pRenderer->setFrustumCullingState(FrustumCulling);

            bool PVS = m_pRenderer->getPVSState();
            ImGui::Checkbox(u8"PVS�޳�", &PVS);
            m_pRenderer->setPVSState(PVS);
            ImGui::EndGroup();
            ImGui::Unindent();
        }

        std::optional<uint32_t> CameraNodeIndex = m_pRenderer->getCameraNodeIndex();
        if (CameraNodeIndex == std::nullopt)
            ImGui::Text(u8"��������ڵ㣺-");
        else
            ImGui::Text((u8"��������ڵ㣺" + std::to_string(CameraNodeIndex.value())).c_str());

        if (!m_pRenderer->getBSPState())
        {
            std::set<size_t> RenderObjectList = m_pRenderer->getRenderedObjectSet();
            ImGui::Text((u8"��Ⱦ��������" + std::to_string(RenderObjectList.size())).c_str());
            if (!RenderObjectList.empty())
            {
                std::string RenderNodeListStr = "";
                for (size_t ObjectIndex : RenderObjectList)
                {
                    RenderNodeListStr += std::to_string(ObjectIndex) + ", ";
                }
                ImGui::TextWrapped((u8"��Ⱦ���壺" + RenderNodeListStr).c_str());
            }
        }
        else
        {
            std::set<uint32_t> RenderNodeList = m_pRenderer->getRenderedNodeList();
            ImGui::Text((u8"��Ⱦ�ڵ�����" + std::to_string(RenderNodeList.size())).c_str());
            if (!RenderNodeList.empty())
            {
                std::string RenderNodeListStr = "";
                for (uint32_t NodeIndex : RenderNodeList)
                {
                    RenderNodeListStr += std::to_string(NodeIndex) + ", ";
                }
                ImGui::TextWrapped((u8"��Ⱦ�ڵ㣺" + RenderNodeListStr).c_str());
            }
        }
    }
}