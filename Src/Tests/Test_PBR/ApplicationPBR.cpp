#include "ApplicationPBR.h"
#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

void CApplicationPBR::_initV()
{
    vk::SAppInfo AppInfo = getAppInfo();

    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    m_pCamera = make<CCamera>();
    m_pInteractor = make<CInteractor>(); 
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pRenderPassFullScreen = make<CRenderPassFullScreen>(); 
    m_pRenderPassFullScreen->init(AppInfo, vk::ERenderPassPos::BEGIN);
    m_pPipelineEnv = m_pRenderPassFullScreen->initPipeline<CPipelineEnvironment>();

    CIOImage::Ptr pSkyIOImage = make<CIOImage>("./textures/old_hall_4k.exr");
    pSkyIOImage->read();

    m_pPipelineEnv->setEnvironmentMap(pSkyIOImage);
    
    m_pRenderPassPBR = make<CRenderPassPBR>();
    m_pRenderPassPBR->init(AppInfo, vk::ERenderPassPos::MIDDLE);
    m_pRenderPassPBR->setCamera(m_pCamera);

    m_pGUI = make<CGUIRenderPass>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->init(AppInfo, vk::ERenderPassPos::END);

    m_pCamera->setPos(glm::vec3(0.0f, -13.6114998f, 0.0f));
    m_pCamera->setPhi(90);
    m_pCamera->setTheta(90);
}

void CApplicationPBR::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPipelineEnv->updateUniformBuffer(vImageIndex, m_pCamera);
    m_pRenderPassFullScreen->update(vImageIndex);
    m_pRenderPassPBR->update(vImageIndex);
    m_pGUI->update(vImageIndex);
}

void CApplicationPBR::_renderUIV()
{
    UI::beginFrame(u8"PBR and IBL");
    m_pRenderPassPBR->getCamera()->renderUI();
    m_pInteractor->renderUI();
    m_pRenderPassPBR->renderUI();
    m_pRenderPassFullScreen->renderUI();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationPBR::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SkyBuffers = m_pRenderPassFullScreen->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> SceneBuffers = m_pRenderPassPBR->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SkyBuffers;
    Result.insert(Result.end(), SceneBuffers.begin(), SceneBuffers.end());
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationPBR::_createOtherResourceV()
{
    _recreateOtherResourceV();
}

void CApplicationPBR::_recreateOtherResourceV()
{
    auto ImageFormat = m_pSwapchain->getImageFormat();
    auto Extent = m_pSwapchain->getExtent();
    auto ImageNum = m_pSwapchain->getImageNum();

    m_pRenderPassFullScreen->recreate(ImageFormat, Extent, ImageNum);
    m_pRenderPassPBR->recreate(ImageFormat, Extent, ImageNum);
    m_pGUI->recreate(ImageFormat, Extent, ImageNum);

    __linkPasses();
}

void CApplicationPBR::_destroyOtherResourceV()
{
    m_pPipelineEnv->destroy();
    m_pRenderPassFullScreen->destroy();
    m_pRenderPassPBR->destroy();
    m_pGUI->destroy();

    cleanGlobalCommandBuffer();
}

void CApplicationPBR::__linkPasses()
{
    auto pLinkPBR = m_pRenderPassPBR->getLink();
    auto pLinkEnv = m_pRenderPassFullScreen->getLink();
    auto pLinkGui = m_pGUI->getLink();

    const auto& ImageViews = m_pSwapchain->getImageViews();
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        pLinkEnv->link("Main", ImageViews[i], EPortType::OUTPUT, i);
        pLinkPBR->link("Main", ImageViews[i], EPortType::INPUT, i);
        pLinkPBR->link("Main", ImageViews[i], EPortType::OUTPUT, i);
        pLinkGui->link("Main", ImageViews[i], EPortType::INPUT, i);
        pLinkGui->link("Main", ImageViews[i], EPortType::OUTPUT, i);
    }
}