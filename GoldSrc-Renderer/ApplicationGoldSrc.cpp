#include "ApplicationGoldSrc.h"
#include "ApplicationGoldSrc.h"
#include "PassGoldSrc.h"
#include "PassSimple.h"
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
    m_pPassGUI->update(vImageIndex);
    m_pPassScene->update(vImageIndex);
}

std::vector<VkCommandBuffer> CApplicationGoldSrc::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SceneBuffers = m_pPassScene->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationGoldSrc::_renderUIV()
{
    m_pPassGUI->beginFrame();
    m_pMainUI->renderUI();
    m_pPassGUI->endFrame();
}

void CApplicationGoldSrc::_createOtherResourceV()
{
    Vulkan::SVulkanAppInfo AppInfo = getAppInfo();

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pPassGUI = make<CGUIRenderPass>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo, vk::ERenderPassPos::END);

    m_pMainUI = make<CGUIMain>();
    m_pMainUI->setInteractor(m_pInteractor);
    m_pMainUI->setChangeRendererCallback([this](ERenderMethod vMethod)
    {
        __recreateRenderer(vMethod);
        __linkPasses();
    });

    m_pMainUI->setReadSceneCallback([this](ptr<SScene> vScene)
    {
        m_pScene = vScene;
        m_pPassScene->loadScene(vScene);
    });
    m_pMainUI->setRenderSettingCallback([this]()
    {
        if (m_pCamera) m_pCamera->renderUI();
        if (m_pInteractor) m_pInteractor->renderUI();
        if (m_pPassScene) m_pPassScene->renderUI();
    });

    _recreateOtherResourceV();
}

void CApplicationGoldSrc::_recreateOtherResourceV()
{
    if (m_pPassScene)
        m_pPassScene->destroy();
    __recreateRenderer(ERenderMethod::BSP);
    m_pPassGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    __linkPasses();
}

void CApplicationGoldSrc::_destroyOtherResourceV()
{
    m_pPassScene->destroy();
    m_pPassGUI->destroy();
}

void CApplicationGoldSrc::__recreateRenderer(ERenderMethod vMethod)
{
    m_pDevice->waitUntilIdle();
    if (m_pPassScene)
        m_pPassScene->destroy();

    auto AppInfo = getAppInfo();
    switch (vMethod)
    {
    case ERenderMethod::DEFAULT:
    {
        m_pPassScene = make<CSceneSimpleRenderPass>();
        m_pPassScene->init(AppInfo, vk::ERenderPassPos::BEGIN);
        m_pPassScene->setCamera(m_pCamera);
        break;
    }
    case ERenderMethod::BSP:
    {
        m_pPassScene = make<CSceneGoldSrcRenderPass>();
        m_pPassScene->init(AppInfo, vk::ERenderPassPos::BEGIN);
        m_pPassScene->setCamera(m_pCamera);
        break;
    }
    default:
        break;
    }

    if (m_pScene)
        m_pPassScene->loadScene(m_pScene);
}

void CApplicationGoldSrc::__linkPasses()
{
    auto pLinkScene = m_pPassScene->getLink();
    auto pLinkGui = m_pPassGUI->getLink();

    const auto& ImageViews = m_pSwapchain->getImageViews();
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        pLinkScene->link("Output", ImageViews[i], EPortType::OUTPUT, i);
        pLinkGui->link("Input", ImageViews[i], EPortType::INPUT, i);
        pLinkGui->link("Output", ImageViews[i], EPortType::OUTPUT, i);
    }
}