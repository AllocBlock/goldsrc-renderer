#include "ApplicationPBR.h"
#include "GlobalSingleTimeBuffer.h"
#include "UserInterface.h"

void CApplicationPBR::_initV()
{
    Vulkan::SVulkanAppInfo AppInfo = getAppInfo();

    registerGlobalCommandBuffer(m_pDevice->get(), m_pDevice->getGraphicsQueueIndex());

    m_pCamera = make<CCamera>();
    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pRenderPassFullScreen = make<CRenderPassFullScreen>();
    m_pRenderPassFullScreen->init(AppInfo, ERendererPos::BEGIN);
    m_pPipelineEnv = m_pRenderPassFullScreen->initPipeline<CPipelineEnvironment>();

    CIOImage::Ptr pSkyIOImage = make<CIOImage>("./textures/old_hall_4k.exr");
    pSkyIOImage->read();
    m_pPipelineEnv->setEnvironmentMap(pSkyIOImage);
    
    m_pRenderPassPBR = make<CRenderPassPBR>();
    m_pRenderPassPBR->init(AppInfo, ERendererPos::MIDDLE);
    m_pRenderPassPBR->setCamera(m_pCamera);

    m_pGUI = make<CGUIRenderer>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->init(AppInfo, ERendererPos::END);

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
    m_pGUI->beginFrame(u8"PBR and IBL");
    m_pRenderPassPBR->getCamera()->renderUI();
    m_pInteractor->renderUI();
    m_pRenderPassPBR->renderUI();
    m_pRenderPassFullScreen->renderUI();
    m_pGUI->endFrame();
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
    auto ImageFormat = m_pSwapchain->getImageFormat();
    auto Extent = m_pSwapchain->getExtent();
    auto ImageViewSet = m_pSwapchain->getImageViews();

    m_pRenderPassFullScreen->recreate(ImageFormat, Extent, ImageViewSet);
    m_pRenderPassPBR->recreate(ImageFormat, Extent, ImageViewSet);
    m_pGUI->recreate(ImageFormat, Extent, ImageViewSet);
}

void CApplicationPBR::_destroyOtherResourceV()
{
    m_pPipelineEnv->destroy();
    m_pRenderPassFullScreen->destroy();
    m_pRenderPassPBR->destroy();
    m_pGUI->destroy();

    unregisterGlobalCommandBuffer();
}