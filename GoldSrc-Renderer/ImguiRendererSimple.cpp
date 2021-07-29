#include "ImguiRendererSimple.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void CImguiRendererSimple::_setTargetV(std::shared_ptr<CRendererScene> vRenderer)
{
    m_pRenderer = std::static_pointer_cast<CRendererSceneSimple>(vRenderer);
}

void CImguiRendererSimple::_drawV()
{
    if (!m_pRenderer) return;
    if (ImGui::CollapsingHeader(u8"äÖÈ¾Æ÷ÉèÖÃ", ImGuiTreeNodeFlags_DefaultOpen))
    {
        bool SkyRendering = m_pRenderer->getSkyState();
        ImGui::Checkbox(u8"¿ªÆôÌì¿ÕäÖÈ¾", &SkyRendering);
        m_pRenderer->setSkyState(SkyRendering);

        bool Culling = m_pRenderer->getCullingState();
        ImGui::Checkbox(u8"¿ªÆôÌÞ³ý", &Culling);
        m_pRenderer->setCullingState(Culling);
        Culling = m_pRenderer->getCullingState();

        if (Culling)
        {
            ImGui::Indent(20.0f);
            ImGui::BeginGroup();
            bool FrustumCulling = m_pRenderer->getFrustumCullingState();
            ImGui::Checkbox(u8"CPUÊÓ×¶ÌÞ³ý", &FrustumCulling);
            m_pRenderer->setFrustumCullingState(FrustumCulling);
            ImGui::EndGroup();
            ImGui::Unindent();
        }
    }
}