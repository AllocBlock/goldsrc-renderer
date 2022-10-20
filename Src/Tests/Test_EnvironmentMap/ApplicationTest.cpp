#include "ApplicationTest.h"

#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

using namespace vk;

void CApplicationTest::_initV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassMain->update(vImageIndex);
    m_pPassGUI->update(vImageIndex);
}

void CApplicationTest::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"ª∑æ≥Ã˘Õº Environment Mapping");
    UI::text(u8"≤‚ ‘");
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SceneBuffers = m_pPassMain->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationTest::_createOtherResourceV()
{
    vk::SAppInfo AppInfo = getAppInfo();

    m_pPassMain = make<CRenderPassTest>();
    m_pPassMain->init(AppInfo);

    m_pPassGUI = make<CRenderPassGUI>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pPassMain->getCamera());

    __linkPasses();
}

void CApplicationTest::_destroyOtherResourceV()
{
    destroyAndClear(m_pPassGUI);
    destroyAndClear(m_pPassMain);
    cleanGlobalCommandBuffer();
}

void CApplicationTest::__linkPasses()
{
    auto pPortMain = m_pPassMain->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortMain, "Main");;
    CPortSet::link(pPortMain, "Main", pPortGui, "Main");
    m_pSwapchainPort->setForceNotReady(false);
}