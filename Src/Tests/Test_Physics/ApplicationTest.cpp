#include "ApplicationTest.h"
#include "GlobalSingleTimeBuffer.h"
#include "Gui.h"

using namespace vk;

void CApplicationTest::_initV()
{
    setupGlobalCommandBuffer(m_pDevice, m_pDevice->getGraphicsQueueIndex());

    auto AppInfo = getAppInfo();

    m_pCamera = make<CCamera>();
    m_pCamera->setFov(90);
    m_pCamera->setAspect(AppInfo.Extent.width / AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(10.0, 10.0, 10.0));
    m_pCamera->setAt(glm::vec3(0.0, 0.0, 0.0));
}

void CApplicationTest::_updateV(uint32_t vImageIndex)
{
    m_pInteractor->update();
    m_pPassGUI->update(vImageIndex);
    m_pPassShade->update(vImageIndex);
}

void CApplicationTest::_renderUIV()
{
    UI::beginFrame();
    UI::beginWindow(u8"物理系统 Physics");
    UI::text(u8"测试");
    m_pInteractor->getCamera()->renderUI();
    UI::endWindow();
    UI::endFrame();
}

std::vector<VkCommandBuffer> CApplicationTest::_getCommandBufferSetV(uint32_t vImageIndex)
{
    std::vector<VkCommandBuffer> ShadeBuffers = m_pPassShade->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> GUIBuffers = m_pPassGUI->requestCommandBuffers(vImageIndex);
    std::vector<VkCommandBuffer> Result = ShadeBuffers;
    Result.insert(Result.end(), GUIBuffers.begin(), GUIBuffers.end());
    return Result;
}

void CApplicationTest::_createOtherResourceV()
{
    vk::SAppInfo AppInfo = getAppInfo();

    m_pInteractor = make<CInteractor>();
    m_pInteractor->bindEvent(m_pWindow, m_pCamera);

    m_pPassGUI = make<CGUIRenderPass>();
    m_pPassGUI->setWindow(m_pWindow);
    m_pPassGUI->init(AppInfo);

    m_pPassShade = make<CRenderPassShade>();
    m_pPassShade->init(AppInfo);
    m_pPassShade->setCamera(m_pCamera);
    
    __generateScene();
    m_pPassShade->setScene(m_ObjectSet);
    
    __linkPasses();
}

void CApplicationTest::_recreateOtherResourceV()
{
}

void CApplicationTest::_destroyOtherResourceV()
{
    destroyAndClear(m_pPassGUI);
    destroyAndClear(m_pPassShade);

    cleanGlobalCommandBuffer();
}

void CApplicationTest::__linkPasses()
{
    auto pPortShade = m_pPassShade->getPortSet();
    auto pPortGui = m_pPassGUI->getPortSet();

    m_pSwapchainPort->setForceNotReady(true);
    CPortSet::link(m_pSwapchainPort, pPortShade, "Main");;
    CPortSet::link(pPortShade,  "Main", pPortGui, "Main");
    m_pSwapchainPort->setForceNotReady(false);
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
    for (size_t i = 0; i < IndexSet.size(); ++i)
    {
        size_t Index = IndexSet[i];
        pVertexArray->append(VertexSet[Index]);
        pNormalArray->append(NormalSet[i / 6]);
    }

    auto pObject = make<C3DObject>();
    pObject->setVertexArray(pVertexArray);
    pObject->setNormalArray(pNormalArray);
    return pObject;
}
