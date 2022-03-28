#include "ApplicationGoldSrc.h"
#include "ApplicationGoldSrc.h"
#include "VulkanRenderer.h"
#include "SimpleRenderer.h"
#include "Common.h"

#include <iostream>
#include <set>

void CApplicationGoldSrc::_initV()
{
    m_pCamera = make<CCamera>();
}

void CApplicationGoldSrc::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pGUI->update(vImageIndex);
    m_pRenderer->update(vImageIndex);
}

std::vector<VkCommandBuffer> CApplicationGoldSrc::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SceneBuffers = m_pRenderer->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationGoldSrc::_renderUIV()
{
    m_pGUI->beginFrame(u8"PBR and IBL");
    m_pMainUI->renderUI();
    m_pGUI->endFrame();
}

void CApplicationGoldSrc::_createOtherResourceV()
{
    Vulkan::SVulkanAppInfo AppInfo = getAppInfo();

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pGUI = make<CGUIRenderer>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->init(AppInfo, ERendererPos::END);

    m_pMainUI = make<CGUIMain>();
    m_pMainUI->setInteractor(m_pInteractor);
    m_pMainUI->setChangeRendererCallback([this](ERenderMethod vMethod)
    {
        __recreateRenderer(vMethod);
    });

    m_pMainUI->setReadSceneCallback([this](ptr<SScene> vScene)
    {
        m_pScene = vScene;
        m_pRenderer->loadScene(vScene);
    });
    m_pMainUI->setRenderSettingCallback([this]()
    {
        if (m_pCamera) m_pCamera->renderUI();
        if (m_pInteractor) m_pInteractor->renderUI();
        if (m_pRenderer) m_pRenderer->renderUI();
    });

    _recreateOtherResourceV();
}

void CApplicationGoldSrc::_recreateOtherResourceV()
{
    if (m_pRenderer)
        m_pRenderer->destroy();
    __recreateRenderer(ERenderMethod::BSP);
    m_pGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageViews());
}

void CApplicationGoldSrc::_destroyOtherResourceV()
{
    m_pRenderer->destroy();
    m_pGUI->destroy();
}

void CApplicationGoldSrc::__recreateRenderer(ERenderMethod vMethod)
{
    m_pDevice->waitUntilIdle();
    if (m_pRenderer)
        m_pRenderer->destroy();

    auto AppInfo = getAppInfo();
    switch (vMethod)
    {
    case ERenderMethod::DEFAULT:
    {
        m_pRenderer = make<CRendererSceneSimple>();
        m_pRenderer->init(AppInfo, ERendererPos::BEGIN);
        m_pRenderer->setCamera(m_pCamera);
        break;
    }
    case ERenderMethod::BSP:
    {
        m_pRenderer = make<CRendererSceneGoldSrc>();
        m_pRenderer->init(AppInfo, ERendererPos::BEGIN);
        m_pRenderer->setCamera(m_pCamera);
        break;
    }
    default:
        break;
    }

    if (m_pScene)
        m_pRenderer->loadScene(m_pScene);
}