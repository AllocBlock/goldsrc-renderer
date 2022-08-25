#include "UserInterface.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

void UI::text(std::string vName, bool vWarp)
{
    if (vWarp)
        return ImGui::TextWrapped(vName.c_str());
    else
        return ImGui::Text(vName.c_str());
}

void UI::bulletText(std::string vName)
{
    return ImGui::BulletText(vName.c_str());
}

bool UI::input(std::string vName, float& vioValue, float vStep)
{
    return ImGui::InputFloat(vName.c_str(), &vioValue, vStep);
}

bool UI::drag(std::string vName, float& vioValue, float vStep, float vMin, float vMax)
{
    return ImGui::DragFloat(vName.c_str(), &vioValue, vStep, vMin, vMax);
}

bool UI::drag(std::string vName, glm::vec3& vioValue, float vStep, float vMin, float vMax)
{
    return ImGui::DragFloat3(vName.c_str(), glm::value_ptr(vioValue), vStep, vMin, vMax);
}

bool UI::slider(std::string vName, float& vioValue, float vMin, float vMax, std::string vFormat)
{
    return ImGui::SliderFloat(vName.c_str(), &vioValue, vMin, vMax, vFormat.c_str());
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

bool UI::button(std::string vName)
{
    return ImGui::Button(vName.c_str());
}

bool UI::collapse(std::string vName, bool vDefaultOpen)
{
    return ImGui::CollapsingHeader(vName.c_str(), vDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
}

void UI::beginGroup() { ImGui::BeginGroup(); }
void UI::endGroup() { ImGui::EndGroup(); }
void UI::sameLine() { ImGui::SameLine(); }
void UI::indent(float vWidth) { ImGui::Indent(vWidth); }
void UI::unindent() { ImGui::Unindent(); }
void UI::spacing() { ImGui::Spacing(); }
void UI::split() { ImGui::Separator(); }

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