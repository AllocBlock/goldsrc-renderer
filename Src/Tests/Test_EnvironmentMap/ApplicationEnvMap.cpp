#include "ApplicationEnvMap.h"

#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

using namespace vk;

void CApplicationEnvMap::_createV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    vk::SAppInfo AppInfo = getAppInfo();

    m_pPassMain = make<CRenderPassSprite>();
    m_pPassMain->init(AppInfo);
    auto pCamera = m_pPassMain->getCamera();

    m_pPassGUI = make<CRenderPassGUI>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, pCamera);

    __linkPasses();

    pCamera->setPos(glm::vec3(2.0f));
    pCamera->setAt(glm::vec3(0.0f));
}

void CApplicationEnvMap::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassMain->update(vImageIndex);
    m_pPassGUI->update(vImageIndex);
}

void CApplicationEnvMap::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"»·¾³ÌùÍ¼ Environment Mapping");
    m_pInteractor->getCamera()->renderUI();
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationEnvMap::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SceneBuffers = m_pPassMain->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationEnvMap::_destroyV()
{
    destroyAndClear(m_pPassGUI);
    destroyAndClear(m_pPassMain);

    cleanGlobalCommandBuffer();
}

void CApplicationEnvMap::__linkPasses()
{
    auto pPortMain = m_pPassMain->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortMain, "Main");;
    CPortSet::link(pPortMain, "Main", pPortGui, "Main");
    m_pSwapchainPort->setForceNotReady(false);
}