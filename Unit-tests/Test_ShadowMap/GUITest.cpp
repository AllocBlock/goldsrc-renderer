#include "GUITest.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void CGUITest::_updateV(uint32_t vImageIndex)
{
    __drawGUI();
}

void CGUITest::__drawGUI()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin(u8"阴影映射 Shadow Map");
    ImGui::Text(u8"测试");
    if (m_pCamera)
    {
        if (ImGui::Button(u8"相机移动到原点"))
        {
            m_pCamera->setPos(glm::vec3(0.0, 0.0, 0.0));
        }

        static float At[3];
        if (ImGui::DragFloat3(u8"看向", At, 0.05, -100, 100))
        {
            m_pCamera->setAt(glm::vec3(At[0], At[1], At[2]));
        }
    }
    if (ImGui::Button(u8"导出ShadowMap图片"))
    {
        m_pRenderer->exportShadowMapToFile("shadowmap.ppm");
    }
    ImGui::End();
    ImGui::Render();
}