#include "ApplicationEnvMap.h"

#include "SingleTimeCommandBuffer.h"
#include "..\..\Gui\InterfaceGui.h"

using namespace vk;

void CApplicationEnvMap::_createV()
{
    SingleTimeCommandBuffer::setup(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    VkExtent2D ScreenExtent = m_pSwapchain->getExtent();

    m_pPassMain = make<CRenderPassSprite>();
    m_pPassMain->createPortSet();
    m_pPassGui = make<CRenderPassGUI>(m_pWindow);
    m_pPassGui->createPortSet();
    m_pPassPresent = make<CRenderPassPresent>(m_pSwapchain);
    m_pPassPresent->createPortSet();

    __linkPasses();

    m_pPassMain->init(m_pDevice, ScreenExtent);
    m_pPassGui->init(m_pDevice, ScreenExtent);
    m_pPassPresent->init(m_pDevice, ScreenExtent);

    auto pCamera = m_pPassMain->getCamera();
    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, pCamera);
    pCamera->setPos(glm::vec3(2.0f));
    pCamera->setAt(glm::vec3(0.0f));
}

void CApplicationEnvMap::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassMain->update();
    m_pPassGui->update();
    m_pPassPresent->updateSwapchainImageIndex(vImageIndex);
    m_pPassPresent->update();
}

void CApplicationEnvMap::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"»·¾³ÌùÍ¼ Environment Mapping");
    m_pInteractor->getCamera()->renderUI();
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationEnvMap::_getCommandBuffers()
{
    std::vector<sptr<engine::IRenderPass>> Passes = { m_pPassMain, m_pPassGui, m_pPassPresent };
    auto buffers = std::vector<VkCommandBuffer>();
    for (const auto& pPass : Passes)
    {
        const auto& subBuffers = pPass->requestCommandBuffers();
        buffers.insert(buffers.end(), subBuffers.begin(), subBuffers.end());
    }
    return buffers;
}

void CApplicationEnvMap::_destroyV()
{
    destroyAndClear(m_pPassMain);
    destroyAndClear(m_pPassGui);
    destroyAndClear(m_pPassPresent);

    SingleTimeCommandBuffer::clean();
}

void CApplicationEnvMap::__linkPasses()
{
    auto pPortMain = m_pPassMain->getPortSet();
    auto pPortGui = m_pPassGui->getPortSet();
    auto pPortPresent = m_pPassPresent->getPortSet();

    CPortSet::link(pPortMain, "Main", pPortGui, "Main");
    CPortSet::link(pPortGui, "Main", pPortPresent, "Main");
}