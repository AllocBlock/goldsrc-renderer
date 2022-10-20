#include "ApplicationTest.h"
#include "InterfaceUI.h"

using namespace vk;

void CApplicationTest::_initV()
{
    vk::SAppInfo AppInfo = getAppInfo();

    m_pRenderPass = make<CRendererTest>();
    m_pRenderPass->init(AppInfo, ERenderPassPos::BEGIN);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pRenderPass->getCamera());

    m_pPassGUI = make<CGUIRenderPass>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo, ERenderPassPos::END);
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pRenderPass->update(vImageIndex);
}

void CApplicationTest::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"SpräÖÈ¾");
    UI::text(u8"²âÊÔ");
    m_pInteractor->getCamera()->renderUI();
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> BufferSet;
    std::vector<VkCommandBuffer> RendererBufferSet = m_pRenderPass->requestCommandBuffers(vImageIndex);
    BufferSet.insert(BufferSet.end(), RendererBufferSet.begin(), RendererBufferSet.end());
    std::vector<VkCommandBuffer> GUIBufferSet = m_pPassGUI->requestCommandBuffers(vImageIndex);
    BufferSet.insert(BufferSet.end(), GUIBufferSet.begin(), GUIBufferSet.end());
    return BufferSet;
}

void CApplicationTest::_createOtherResourceV()
{
    m_pPassGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    m_pRenderPass->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    __linkPasses();
}

void CApplicationTest::_recreateOtherResourceV()
{
    _createOtherResourceV();
}

void CApplicationTest::_destroyOtherResourceV()
{
    m_pPassGUI->destroy();
    m_pRenderPass->destroy();
}

void CApplicationTest::__linkPasses()
{
    auto pLinkMain = m_pRenderPass->getLink();
    auto pLinkGui = m_pPassGUI->getLink();

    const auto& ImageViews = m_pSwapchain->getImageViews();
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        pLinkMain->link("Main", ImageViews[i], EPortType::OUTPUT, i);
        pLinkGui->link("Main", ImageViews[i], EPortType::INPUT, i);
        pLinkGui->link("Main", ImageViews[i], EPortType::OUTPUT, i);
    }
}
