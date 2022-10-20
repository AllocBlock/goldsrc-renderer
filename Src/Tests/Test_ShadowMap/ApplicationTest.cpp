#include "ApplicationTest.h"
#include "GlobalSingleTimeBuffer.h"
#include "InterfaceUI.h"

using namespace vk;

void CApplicationTest::_initV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    vk::SAppInfo AppInfo = getAppInfo();

    m_pRenderPassShadowMap = make<CRenderPassShadowMap>();
    m_pRenderPassShadowMap->init(AppInfo, ERenderPassPos::BEGIN);

    m_pPassShade = make<CRenderPassShade>();
    m_pPassShade->init(AppInfo, ERenderPassPos::BEGIN);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pPassShade->getCamera());

    m_pPassGUI = make<CGUIRenderPass>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo, ERenderPassPos::END);

    __generateScene();
    m_pPassShade->setScene(m_ObjectSet);
    m_pRenderPassShadowMap->setScene(m_ObjectSet);
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pRenderPassShadowMap->update(vImageIndex);
    m_pPassShade->update(vImageIndex);
}

void CApplicationTest::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"ÒõÓ°Ó³Éä Shadow Map");
    UI::text(u8"²âÊÔ");
    m_pInteractor->getCamera()->renderUI();
    if (UI::button(u8"µ¼³öShadowMapÍ¼Æ¬"))
    {
        m_pRenderPassShadowMap->exportShadowMapToFile("shadowmap.ppm");
    }
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> ShadowMapBuffers = m_pRenderPassShadowMap->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> ShadeBuffers = m_pPassShade->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = ShadowMapBuffers;
    Result.insert(Result.end(), ShadeBuffers.begin(), ShadeBuffers.end());
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationTest::_createOtherResourceV()
{
    m_pPassGUI->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    m_pRenderPassShadowMap->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());
    m_pPassShade->recreate(m_pSwapchain->getImageFormat(), m_pSwapchain->getExtent(), m_pSwapchain->getImageNum());

    __linkPasses();
}

void CApplicationTest::_recreateOtherResourceV()
{
}

void CApplicationTest::_destroyOtherResourceV()
{
    m_pPassGUI->destroy();
    m_pRenderPassShadowMap->destroy();
    m_pPassShade->destroy();

    cleanGlobalCommandBuffer();
}

void CApplicationTest::__linkPasses()
{
    auto pLinkShadowMap = m_pRenderPassShadowMap->getLink();
    auto pLinkShade = m_pPassShade->getLink();
    auto pLinkGui = m_pPassGUI->getLink();

    const auto& ImageViews = m_pSwapchain->getImageViews();
    for (int i = 0; i < m_pSwapchain->getImageNum(); ++i)
    {
        pLinkShade->linkInput("ShadowMap", pLinkShadowMap->getOutput("ShadowMap", i), i);
        pLinkShade->linkOutput("Main", ImageViews[i], i);
        pLinkGui->linkInput("Main", ImageViews[i], i);
        pLinkGui->linkOutput("Main", ImageViews[i], i);
    }

    m_pPassShade->setShadowMapInfo(m_pRenderPassShadowMap->getLightCamera());
}

void CApplicationTest::__generateScene()
{
    // ground
    glm::vec3 Normal = glm::vec3(0.0, 0.0, 1.0);
    std::array<glm::vec3, 6> VertexSet =
    {
        glm::vec3(10,  10, 0),
        glm::vec3(10, -10, 0),
        glm::vec3(-10, -10, 0),
        glm::vec3(10,  10, 0),
        glm::vec3(-10, -10, 0),
        glm::vec3(-10,  10, 0),
    };

    auto pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    auto pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    for (auto& Vertex : VertexSet)
    {
        pVertexArray->append(Vertex);
        pNormalArray->append(Normal);
    }

    auto pObject = make<CGeneralMeshData>();
    pObject->setVertexArray(pVertexArray);
    pObject->setNormalArray(pNormalArray);
    m_ObjectSet.emplace_back(pObject);

    // objects
    m_ObjectSet.emplace_back(__createCube(glm::vec3(0.0, 0.0, 0.0), 5.0f));
    m_ObjectSet.emplace_back(__createCube(glm::vec3(0.0, 3.0, 0.0), 1.0f));
}

ptr<CGeneralMeshData> CApplicationTest::__createCube(glm::vec3 vCenter, float vSize)
{
    /*
     *   4------5      y
     *  /|     /|      |
     * 0------1 |      |
     * | 7----|-6      -----x
     * |/     |/      /
     * 3------2      z
     */
    std::array<glm::vec3, 8> VertexSet =
    {
        glm::vec3(-1,  1,  1),
        glm::vec3(1,  1,  1),
        glm::vec3(1, -1,  1),
        glm::vec3(-1, -1,  1),
        glm::vec3(-1,  1, -1),
        glm::vec3(1,  1, -1),
        glm::vec3(1, -1, -1),
        glm::vec3(-1, -1, -1),
    };

    for (auto& Vertex : VertexSet)
        Vertex = vCenter + Vertex * vSize * 0.5f;

    const std::array<size_t, 36> IndexSet =
    {
        0, 1, 2, 0, 2, 3, // front
        5, 4, 7, 5, 7, 6, // back
        4, 5, 1, 4, 1, 0, // up
        3, 2, 6, 3, 6, 7, // down
        4, 0, 3, 4, 3, 7, // left
        1, 5, 6, 1, 6, 2  // right
    };

    std::array<glm::vec3, 6> NormalSet =
    {
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(1, 0, 0),
    };

    auto pVertexArray = make<CGeneralDataArray<glm::vec3>>();
    auto pNormalArray = make<CGeneralDataArray<glm::vec3>>();
    for (size_t Index : IndexSet)
    {
        pVertexArray->append(VertexSet[Index]);
        pNormalArray->append(NormalSet[Index / 6]);
    }

    auto pObject = make<CGeneralMeshData>();
    pObject->setVertexArray(pVertexArray);
    pObject->setNormalArray(pNormalArray);
    return pObject;
}
