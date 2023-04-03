#pragma once
#include "Vulkan.h"
#include "Image.h"
#include "AppInfo.h"

#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace UI
{
    enum EWindowFlag
    {
        NONE = 0,
        NO_TITLE_BAR = 1 << 0,
        NO_RESIZE = 1 << 1,
        ALWAYS_AUTO_RESIZE = 1 << 2,
        MENU_BAR = 1 << 3,

        ENUM_NUM = 5,
    };

    enum class ESetVariableCondition
    {
        NONE = 0,
        ALWAYS,
        ONCE,
        FIRST_EVER_USE,
        APPEARING
    };

    void init(vk::CDevice::CPtr vDevice, GLFWwindow* vWindow, VkDescriptorPool vPool, uint32_t vImageNum, VkRenderPass vRenderPass);
    void setFont(std::string vFontFile, CCommandBuffer::Ptr vSingleTimeCommandBuffer);
    void destory();
    void draw(CCommandBuffer::Ptr vCommandBuffer);
    void beginFrame(std::string vTitle = u8"Ä¬ÈÏ´°¿Ú");
    void endFrame();

    bool isInited();

    void beginWindow(std::string vTitle, bool* vIsOpen = nullptr, int vFlags = 0);
    void endWindow();
    void text(std::string vName, bool vWarp = false);
    void bulletText(std::string vName);
    bool input(std::string vName, float& vioValue, float vStep = 0.01f);
    bool input(std::string vName, glm::vec3& vioValue);
    bool drag(std::string vName, float& vioValue, float vStep = 0.01f, float vMin = 0.0f, float vMax = 0.0f);
    bool drag(std::string vName, glm::vec3& vioValue, float vStep = 0.01f, float vMin = 0.0f, float vMax = 0.0f);
    bool slider(std::string vName, float& vioValue, float vMin = 0.0f, float vMax = 0.0f, std::string vFormat = "");
    bool inputColor(std::string vName, glm::vec3& vioColor);
    bool inputColor(std::string vName, glm::vec4& vioColor);
    bool toggle(std::string vName, bool& vioValue);
    bool button(std::string vName);
    void plotLines(std::string vName, const std::vector<float>& vData);
    bool combo(std::string vName, const std::vector<const char*>& vItemSet, int& vioIndex);
    bool collapse(std::string vName, bool vDefaultOpen = false);
    void beginGroup();
    void endGroup();
    void openPopup(std::string vTitle);
    bool isPopupOpen(std::string vTitle);
    void closeCurrentPopup();
    bool beginPopupModal(std::string vTitle, bool* vIsOpen = nullptr, int vWindowFlags = 0);
    void endPopup();
    bool treeNode(std::string vName);
    void treePop();
    void image(const vk::CImage& vImage, const glm::vec2& vSize);

    bool beginMenuBar();
    bool beginMenu(std::string vTitle);
    bool menuItem(std::string vTitle, bool* vSelected = nullptr);
    void endMenu();
    void endMenuBar();

    void sameLine();
    void indent(float vWidth);
    void unindent();
    void spacing();
    void split();
    void setNextWindowPos(const glm::vec2& vCenter, ESetVariableCondition vCond, const glm::vec2& vPivot);
    void setScrollHereX(float vCenterRatio = 0.5f);
    void setScrollHereY(float vCenterRatio = 0.5f);

    bool isUsingMouse();
    bool isUsingKeyboard();

    glm::vec2 getDisplaySize();
    glm::vec2 getDisplayCenter();
};

