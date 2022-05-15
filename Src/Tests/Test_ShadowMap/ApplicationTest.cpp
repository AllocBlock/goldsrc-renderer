#include "ApplicationTest.h"
#include "imgui.h"

using namespace vk;

void CApplicationTest::_initV()
{
    vk::SAppInfo AppInfo = getAppInfo();

    m_pRenderPass = make<CRendererTest>();
    m_pRenderPass->init(AppInfo, ERenderPassPos::BEGIN);

    m_pGUIPass = make<CGUIRenderPass>();
    m_pGUIPass->setWindow(m_pWindow);
    m_pGUIPass->init(AppInfo, ERenderPassPos::END);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pRenderPass->getCamera());
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pGUIPass->update(vImageIndex);
    m_pRenderPass->update(vImageIndex);
}

void CApplicationTest::_renderUIV()
{
    m_pGUIPass->beginFrame();
    ImGui::Begin(u8"��Ӱӳ�� Shadow Map");
    ImGui::Text(u8"����");
    m_pInteractor->getCamera()->renderUI();
    if (ImGui::Button(u8"����ShadowMapͼƬ"))
    {
        m_pRenderPass->exportShadowMapToFile("shadowmap.ppm");
    }
    ImGui::End();
    m_pGUIPass->endFrame();
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SceneBuffers = m_pRenderPass->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pGUIPass->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationTest::_createOtherResourceV()
{
    m_pGUIPass->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    m_pRenderPass->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());

    __linkPasses();
}

void CApplicationTest::_recreateOtherResourceV()
{
    _recreateOtherResourceV();
}

void CApplicationTest::_destroyOtherResourceV()
{
    m_pGUIPass->destroy();
    m_pRenderPass->destroy();
}

void CApplicationTest::__linkPasses()
{
    auto pLinkMain = m_pRenderPass->getLink();
    auto pLinkGui = m_pGUIPass->getLink();

    const auto& ImageViews = m_pSwapchain->getImageViews();
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        pLinkMain->link("Output", ImageViews[i], EPortType::OUTPUT, i);
        pLinkGui->link("Input", ImageViews[i], EPortType::INPUT, i);
        pLinkGui->link("Output", ImageViews[i], EPortType::OUTPUT, i);
    }
}
