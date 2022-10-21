#include "ApplicationPBR.h"
#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

void CApplicationPBR::_initV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());
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
    vk::SAppInfo AppInfo = getAppInfo();

    m_pCamera = make<CCamera>();
    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pRenderPassFullScreen = make<CRenderPassFullScreen>();
    m_pRenderPassFullScreen->init(AppInfo);
    m_pPipelineEnv = m_pRenderPassFullScreen->initPipeline<CPipelineEnvironment>();
    
    m_pRenderPassFullScreen->hookPipelineCreate([this]
        {
            CIOImage::Ptr pSkyIOImage = make<CIOImage>("./textures/old_hall_4k.exr");
            pSkyIOImage->read();
            m_pPipelineEnv->setEnvironmentMap(pSkyIOImage);
        });
    

    m_pRenderPassPBR = make<CRenderPassPBR>();
    m_pRenderPassPBR->init(AppInfo);
    m_pRenderPassPBR->setCamera(m_pCamera);

    m_pGUI = make<CRenderPassGUI>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->init(AppInfo);

    m_pCamera->setPos(glm::vec3(0.0f, -13.6114998f, 0.0f));
    m_pCamera->setPhi(90);
    m_pCamera->setTheta(90);

    __linkPasses();
}

void CApplicationPBR::_destroyOtherResourceV()
{
    destroyAndClear(m_pPipelineEnv);
    destroyAndClear(m_pRenderPassFullScreen);
    destroyAndClear(m_pRenderPassPBR);
    destroyAndClear(m_pGUI);

    cleanGlobalCommandBuffer();
}

void CApplicationPBR::__linkPasses()
{
    auto pPortEnv = m_pRenderPassFullScreen->getPortSet();
    auto pPortPBR = m_pRenderPassPBR->getPortSet();
    auto pPortGui = m_pGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortEnv, "Main");;
    CPortSet::link(pPortEnv, "Main", pPortPBR, "Main");
    CPortSet::link(pPortPBR, "Main", pPortGui, "Main");
    m_pSwapchainPort->setForceNotReady(false);
}