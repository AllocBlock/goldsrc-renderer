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
    ImGui::Begin(u8"“ı”∞”≥…‰ Shadow Map");
    ImGui::Text(u8"≤‚ ‘");
    ImGui::End();
    ImGui::Render();
}