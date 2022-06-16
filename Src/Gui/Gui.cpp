#include "Gui.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <stdexcept>
#include <map>
#include "Sampler.h"

bool gIsFrameBeginned = false;
bool gIsInitted = false;
const std::string gChineseFont = "C:/windows/fonts/simhei.ttf";
vk::CSampler gDefaultSampler;

using TextureId_t = VkDescriptorSet;
std::map<VkImageView, TextureId_t> gTextureIdMap;

void __checkIsInitted()
{
    if (!gIsInitted)
        throw std::runtime_error("not initted, please init the gui before begin a frame");
}

void __checkIsFrameBeginned()
{
    if (!gIsFrameBeginned)
        throw std::runtime_error("not in a frame, please start a frame before draw any ui");
}

ImGuiWindowFlags __toImguiWindowFlag(UI::EWindowFlag vFlag)
{
    switch (vFlag)
    {
    case UI::EWindowFlag::NONE: return ImGuiWindowFlags_None;
    case UI::EWindowFlag::NO_TITLE_BAR: return ImGuiWindowFlags_NoTitleBar;
    case UI::EWindowFlag::NO_RESIZE: return ImGuiWindowFlags_NoResize;
    case UI::EWindowFlag::ALWAYS_AUTO_RESIZE: return ImGuiWindowFlags_AlwaysAutoResize;
    case UI::EWindowFlag::MENU_BAR: return ImGuiWindowFlags_MenuBar;
    default: return ImGuiWindowFlags_None;
    }
}

ImGuiWindowFlags __toImguiWindowFlags(int vFlag)
{
    ImGuiWindowFlags Flags = 0;
    int CurFlag = 1;
    int Num = UI::EWindowFlag::ENUM_NUM;
    while(Num--)
    {
        if (vFlag & CurFlag)
        {
            Flags |= __toImguiWindowFlag((UI::EWindowFlag)CurFlag);
        }
        CurFlag <<= 1;
    }
    return Flags;
}

ImGuiCond __toImguiCondFlag(UI::ESetVariableCondition vFlag)
{
    switch (vFlag)
    {
    case UI::ESetVariableCondition::NONE: return ImGuiCond_None;
    case UI::ESetVariableCondition::ALWAYS: return ImGuiCond_Always;
    case UI::ESetVariableCondition::ONCE: return ImGuiCond_Once;
    case UI::ESetVariableCondition::FIRST_EVER_USE: return ImGuiCond_FirstUseEver;
    case UI::ESetVariableCondition::APPEARING: return ImGuiCond_Appearing;
    default: return ImGuiCond_None;
    }
}

ImVec2 __toImguiVec2(glm::vec2 vVec)
{
    return ImVec2(vVec.x, vVec.y);
}

glm::vec2 __toGlmVec2(ImVec2 vVec)
{
    return glm::vec2(vVec.x, vVec.y);
}

void UI::init(const vk::SAppInfo& vAppInfo, GLFWwindow* vWindow, VkDescriptorPool vPool, uint32_t vImageNum, VkCommandBuffer vSingleTimeCommandBuffer,VkRenderPass& voRenderPass)
{
    auto pDevice = vAppInfo.pDevice;

    // setup context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& IO = ImGui::GetIO();
    IO.Fonts->AddFontFromFileTTF(gChineseFont.c_str(), 13.0f, NULL, IO.Fonts->GetGlyphRangesChineseFull());

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForVulkan(vWindow, true);

    // init vulkan
    ImGui_ImplVulkan_InitInfo InitInfo = {};
    InitInfo.Instance = *pDevice->getPhysicalDevice()->getInstance();
    InitInfo.PhysicalDevice = *pDevice->getPhysicalDevice();
    InitInfo.Device = *pDevice;
    InitInfo.QueueFamily = pDevice->getGraphicsQueueIndex();
    InitInfo.Queue = pDevice->getGraphicsQueue();
    InitInfo.PipelineCache = VK_NULL_HANDLE;
    InitInfo.DescriptorPool = vPool;
    InitInfo.Allocator = nullptr;
    InitInfo.MinImageCount = vImageNum;
    InitInfo.ImageCount = vImageNum;
    InitInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&InitInfo, voRenderPass);

    // upload font
    ImGui_ImplVulkan_CreateFontsTexture(vSingleTimeCommandBuffer);

    // init default sampler
    const auto& Properties = pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_NEAREST, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, Properties.limits.maxSamplerAnisotropy
    );
    gDefaultSampler.create(pDevice, SamplerInfo);

    gIsInitted = true;
}

void UI::destory()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    gDefaultSampler.destroy();

    gIsInitted = false;
}

void UI::draw(VkCommandBuffer vCommandBuffer)
{
    auto pDrawData = ImGui::GetDrawData();
    if (pDrawData)
        ImGui_ImplVulkan_RenderDrawData(pDrawData, vCommandBuffer);
}

void UI::beginFrame(std::string vTitle)
{
    __checkIsInitted();
    if (gIsFrameBeginned) throw "Already in a frame";
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    UI::beginWindow(vTitle);
    gIsFrameBeginned = true;
}

void UI::endFrame()
{
    if (!gIsFrameBeginned) throw "Already out of a frame";
    UI::endWindow();
    ImGui::Render();
    gIsFrameBeginned = false;
}

void UI::beginWindow(std::string vTitle, bool* vIsOpen, int vWindowFlags)
{
    ImGui::Begin(vTitle.c_str(), vIsOpen, __toImguiWindowFlags(vWindowFlags));
}

void UI::endWindow()
{
    ImGui::End();
}

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

void UI::plotLines(std::string vName, const std::vector<float>& vData)
{
    ImGui::PlotLines(vName.c_str(), vData.data(), vData.size());
}

bool UI::combo(std::string vName, const std::vector<const char*>& vItemSet, int& vioIndex)
{
    return ImGui::Combo(vName.c_str(), &vioIndex, vItemSet.data(), static_cast<int>(vItemSet.size()));
}

bool UI::collapse(std::string vName, bool vDefaultOpen)
{
    return ImGui::CollapsingHeader(vName.c_str(), vDefaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
}

void UI::beginGroup() { ImGui::BeginGroup(); }
void UI::endGroup() { ImGui::EndGroup(); }
void UI::openPopup(std::string vTitle) { ImGui::OpenPopup(vTitle.c_str()); }
void UI::closeCurrentPopup() { ImGui::CloseCurrentPopup(); }
bool UI::beginPopupModal(std::string vTitle, bool* vIsOpen, int vWindowFlags)
{
    return ImGui::BeginPopupModal(vTitle.c_str(), vIsOpen, vWindowFlags);
}
void UI::endPopup() { ImGui::EndPopup(); }
bool UI::isPopupOpen(std::string vTitle) { return ImGui::IsPopupOpen(vTitle.c_str()); }
bool UI::treeNode(std::string vName) { return ImGui::TreeNode(vName.c_str()); }
void UI::treePop() { ImGui::TreePop(); }
void UI::image(vk::CImage::CPtr vImage, const glm::vec2& vSize) 
{ 
    TextureId_t TextureId = 0;

    VkImageView ImageView = vImage->get();
    auto pItem = gTextureIdMap.find(ImageView);
    if (pItem != gTextureIdMap.end())
    {
        TextureId = pItem->second;
    }
    else
    {
        TextureId = ImGui_ImplVulkan_AddTexture(gDefaultSampler, vImage->get(), vImage->getLayout());
        gTextureIdMap[ImageView] = TextureId;
    }

    ImGui::Image(TextureId, __toImguiVec2(vSize));
}

bool UI::beginMenuBar() { return ImGui::BeginMenuBar(); }
bool UI::beginMenu(std::string vTitle) { return ImGui::BeginMenu(vTitle.c_str()); }
bool UI::menuItem(std::string vTitle, bool* vSelected) { return ImGui::MenuItem(vTitle.c_str(), nullptr, vSelected); }
void UI::endMenu() { ImGui::EndMenu(); }
void UI::endMenuBar() { ImGui::EndMenuBar(); }

void UI::sameLine() { ImGui::SameLine(); }
void UI::indent(float vWidth) { ImGui::Indent(vWidth); }
void UI::unindent() { ImGui::Unindent(); }
void UI::spacing() { ImGui::Spacing(); }
void UI::split() { ImGui::Separator(); }

void UI::setNextWindowPos(const glm::vec2& vCenter, ESetVariableCondition vCond, const glm::vec2& vPivot)
{
    ImGui::SetNextWindowPos(__toImguiVec2(vCenter), __toImguiCondFlag(vCond), __toImguiVec2(vPivot));
}

void UI::setScrollHereX(float vCenterRatio) { ImGui::SetScrollHereX(vCenterRatio); }
void UI::setScrollHereY(float vCenterRatio) { ImGui::SetScrollHereY(vCenterRatio); }

bool UI::isUsingMouse()
{
    __checkIsInitted();
    ImGuiIO& IO = ImGui::GetIO();
    return IO.WantCaptureMouse;
}

bool UI::isUsingKeyboard()
{
    __checkIsInitted();
    ImGuiIO& IO = ImGui::GetIO();
    return IO.WantCaptureKeyboard;
}

glm::vec2 UI::getDisplaySize()
{
    return __toGlmVec2(ImGui::GetIO().DisplaySize);
}