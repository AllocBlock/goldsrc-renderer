#include "ApplicationTest.h"
#include "imgui.h"
#include "GlobalSingleTimeBuffer.h"

using namespace vk;

void CApplicationTest::_initV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    vk::SAppInfo AppInfo = getAppInfo();

    m_pRenderPass = make<CRendererTest>();
    m_pRenderPass->init(AppInfo, ERenderPassPos::BEGIN);

    m_pGUIPass = make<CGUIRenderPass>();
    m_pGUIPass->setWindow(m_pWindow);
    m_pGUIPass->init(AppInfo, ERenderPassPos::END);

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pRenderPass->getCamera());

    __generateScene();
    m_pRenderPassShade->setScene(m_ObjectSet);

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
    ImGui::Begin(u8"ÒõÓ°Ó³Éä Shadow Map");
    ImGui::Text(u8"²âÊÔ");
    m_pInteractor->getCamera()->renderUI();
    if (ImGui::Button(u8"µ¼³öShadowMapÍ¼Æ¬"))
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

    cleanGlobalCommandBuffer();
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

    auto pObject = make<C3DObject>();
    pObject->setVertexArray(pVertexArray);
    pObject->setNormalArray(pNormalArray);
    m_ObjectSet.emplace_back(pObject);

    // objects
    m_ObjectSet.emplace_back(__createCube(glm::vec3(0.0, 0.0, 0.0), 5.0f));
    m_ObjectSet.emplace_back(__createCube(glm::vec3(0.0, 3.0, 0.0), 1.0f));
}

ptr<C3DObject> CApplicationTest::__createCube(glm::vec3 vCenter, float vSize)
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

    auto pObject = make<C3DObject>();
    pObject->setVertexArray(pVertexArray);
    pObject->setNormalArray(pNormalArray);
    return pObject;
}
