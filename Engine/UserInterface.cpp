#include "UserInterface.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void UI::text(std::string vName)
{
    return ImGui::Text(vName.c_str());
}

bool UI::input(std::string vName, float& vioValue, float vStep)
{
    return ImGui::InputFloat(vName.c_str(), &vioValue, vStep);
}

bool UI::drag(std::string vName, float& vioValue, float vMin, float vMax, float vStep)
{
    return ImGui::DragFloat(vName.c_str(), &vioValue, vStep, vMin, vMax);
}

bool UI::inputColor(std::string vName, glm::vec3& vioColor)
{
    return ImGui::ColorEdit3(vName.c_str(), glm::value_ptr(vioColor));
}

bool UI::inputColor(std::string vName, glm::vec4& vioColor)
{
    return ImGui::ColorEdit4(vName.c_str(), glm::value_ptr(vioColor));
}

bool UI::toggle(std::string vName, bool& vioValue)
{
    return ImGui::Checkbox(vName.c_str(), &vioValue);
}

void UI::indent(float vWidth)
{
    ImGui::Indent(vWidth);
}

void UI::split()
{
    ImGui::Separator();
}

bool UI::isUsingMouse()
{
    ImGuiIO& IO = ImGui::GetIO();
    return IO.WantCaptureMouse;
}

bool UI::isUsingKeyboard()
{
    ImGuiIO& IO = ImGui::GetIO();
    return IO.WantCaptureKeyboard;
}