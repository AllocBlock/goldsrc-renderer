#include "ApplicationTest.h"
#include "UserInterface.h"

void CApplicationTest::_initV()
{
    Vulkan::SVulkanAppInfo AppInfo = getAppInfo();

    m_pGUI = std::make_shared<CGUI>();
    m_pGUI->setWindow(m_pWindow);
    m_pGUI->init(AppInfo, ERendererPos::END);
    m_pRenderer = std::make_shared<CRendererTest>();
    m_pRenderer->init(AppInfo, ERendererPos::BEGIN);
    m_pInteractor = std::make_shared<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pRenderer->getCamera());
    m_pRenderer->getCamera()->setPos(glm::vec3(0.0f, -13.6114998f, 0.0f));
    m_pRenderer->getCamera()->setPhi(90);
    m_pRenderer->getCamera()->setTheta(90);
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pGUI->update(vImageIndex);
    m_pRenderer->update(vImageIndex);
}

void CApplicationTest::_renderUIV(uint32_t vImageIndex)
{
    m_pGUI->beginFrame(u8"PBR and IBL");
    m_pRenderer->renderUI(vImageIndex);
    m_pGUI->endFrame();
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> SceneBuffers = m_pRenderer->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = SceneBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationTest::_createOtherResourceV()
{
    m_pGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageViews());
    m_pRenderer->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageViews());
}

void CApplicationTest::_destroyOtherResourceV()
{
    m_pGUI->destroy();
    m_pRenderer->destroy();
}


SResultReadScene CApplicationTest::__readScene(std::filesystem::path vFilePath)
{
    SResultReadScene Result;
    Result.Succeed = false;
    if (!std::filesystem::exists(vFilePath))
    {
        Result.Message = u8"文件不存在（" + vFilePath.u8string() + u8"）";
    }
    else if (vFilePath.extension() == ".bsp" ||
        vFilePath.extension() == ".rmf" ||
        vFilePath.extension() == ".map" ||
        vFilePath.extension() == ".bsp" ||
        vFilePath.extension() == ".obj" ||
        vFilePath.extension() == ".mdl"
        )
    {
        std::string Extension = vFilePath.extension().string().substr(1);
        Result.Succeed = true;
        Result.pScene = SceneReader::read(Extension, vFilePath);
    }
    else
    {
        Result.Message = u8"不支持的文件格式（" + vFilePath.extension().u8string() + u8"）";
    }
    return Result;
}