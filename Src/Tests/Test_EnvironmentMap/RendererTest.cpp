#include "RendererTest.h"
#include "RenderPassDescriptor.h"
#include "Function.h"

void CRendererTest::_initV()
{
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_AppInfo.Extent.width / m_AppInfo.Extent.height);
    m_pCamera->setPos(glm::vec3(0.0, -1.0, 0.0));

    __loadSkyBox();
    __createRenderPass();
    __createCommandPoolAndBuffers();
    __createVertexBuffer();
    __createRecreateResources();
}

SPortDescriptor CRendererTest::_getPortDescV()
{
    CRenderPassPort Ports;
    Ports.addOutput("Output", m_AppInfo.ImageFormat, m_AppInfo.Extent);
    return Ports;
}

void CRendererTest::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
}

void CRendererTest::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRendererTest::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (m_FramebufferSet.empty() || m_pLink->isUpdated())
    {
        __createFramebuffers();
        m_pLink->setUpdateState(false);
    }

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_CommandName, vImageIndex);

    std::vector<VkClearValue> ClearValueSet(2);
    ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValueSet[1].depthStencil = { 1.0f, 0 };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValueSet);

    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_Pipeline.bind(CommandBuffer, vImageIndex);

        size_t VertexNum = m_PointDataSet.size();
        vkCmdDraw(CommandBuffer, VertexNum, 1, 0, 0);
    }
    
    end();
    return { CommandBuffer };
}

void CRendererTest::_destroyV()
{
    __destroyRecreateResources();
    m_pVertexBuffer->destroy();
    m_Command.clear();

    IRenderPass::_destroyV();
}

void CRendererTest::__createRenderPass()
{
    auto Info = CRenderPassDescriptor::generateSingleSubpassInfo(m_RenderPassPosBitField, m_AppInfo.ImageFormat, VK_FORMAT_D32_SFLOAT);
    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &Info, nullptr, _getPtr()));
}

void CRendererTest::__loadSkyBox()
{
    if (m_SkyFilePrefix.empty()) throw "sky box image file not found";

    std::vector<std::string> Extensions = { ".tga", ".bmp", ".png", ".jpg" };

    bool FoundSkyBoxImages = false;
    for (const std::string& Extension : Extensions)
    {
        if (__readSkyboxImages(m_SkyFilePrefix, Extension))
        {
            FoundSkyBoxImages = true;
            break;
        }
    }
    if (!FoundSkyBoxImages)
        throw "sky box image file not found";
}

bool CRendererTest::__readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension)
{
    // front back up down right left
    std::array<std::string, 6> SkyBoxPostfixes = { "ft", "bk", "up", "dn", "rt", "lf" };
    for (size_t i = 0; i < SkyBoxPostfixes.size(); ++i)
    {
        std::filesystem::path ImagePath = vSkyFilePrefix + SkyBoxPostfixes[i] + vExtension;
        if (std::filesystem::exists(ImagePath))
        {
            m_SkyBoxImageSet[i] = make<CIOImage>();
            m_SkyBoxImageSet[i]->read(ImagePath);
        }
        else
        {
            return false;
        }
    }
    return true;
}

void CRendererTest::__createGraphicsPipeline()
{
    m_Pipeline.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CRendererTest::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_CommandName, m_AppInfo.ImageNum, ECommandBufferLevel::PRIMARY);

    vk::beginSingleTimeBufferFunc_t BeginFunc = [this]() -> VkCommandBuffer
    {
        return m_Command.beginSingleTimeBuffer();
    };
    vk::endSingleTimeBufferFunc_t EndFunc = [this](VkCommandBuffer vCommandBuffer)
    {
        m_Command.endSingleTimeBuffer(vCommandBuffer);
    };
    vk::setSingleTimeBufferFunc(BeginFunc, EndFunc);
}

void CRendererTest::__createDepthResources()
{
    m_pDepthImage = Function::createDepthImage(m_AppInfo.pDevice, m_AppInfo.Extent);
}

void CRendererTest::__createFramebuffers()
{
    size_t ImageNum = m_AppInfo.ImageNum;
    m_FramebufferSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pLink->getOutput("Output", i),
            *m_pDepthImage
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CRendererTest::__createVertexBuffer()
{
     __generateScene();
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(STestPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}

void CRendererTest::__createRecreateResources()
{
    __createGraphicsPipeline();
    __createDepthResources();
    m_Pipeline.setImageNum(m_AppInfo.ImageNum);
    m_Pipeline.setSkyBoxImage(m_SkyBoxImageSet);
}

void CRendererTest::__destroyRecreateResources()
{
    m_pDepthImage->destroy();

    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();
    m_Pipeline.destroy();
}

void CRendererTest::__generateScene()
{
    float Sqrt2 = std::sqrt(2);
    float Sqrt3 = std::sqrt(3);
    float Sqrt6 = std::sqrt(6);
    float OneThrid = 1.0 / 3.0;
    std::array<glm::vec3, 4> VertexSet =
    {
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0, 2 * OneThrid * Sqrt2, -OneThrid),
        glm::vec3(OneThrid * Sqrt6, -OneThrid * Sqrt2, -OneThrid),
        glm::vec3(-OneThrid * Sqrt6, -OneThrid * Sqrt2, -OneThrid),
    };

    __subdivideTriangle({ VertexSet[0], VertexSet[1], VertexSet[2] }, 4);
    __subdivideTriangle({ VertexSet[0], VertexSet[2], VertexSet[3] }, 4);
    __subdivideTriangle({ VertexSet[0], VertexSet[3], VertexSet[1] }, 4);
    __subdivideTriangle({ VertexSet[3], VertexSet[2], VertexSet[1] }, 4);
}

void CRendererTest::__subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, int vDepth)
{
    if (vDepth == 0)
    {
        for (const auto& Vertex : vVertexSet)
            m_PointDataSet.emplace_back(STestPointData({ Vertex, Vertex }));
    }
    else
    {
        glm::vec3 Middle01 = glm::normalize(vVertexSet[0] + vVertexSet[1]);
        glm::vec3 Middle12 = glm::normalize(vVertexSet[1] + vVertexSet[2]);
        glm::vec3 Middle20 = glm::normalize(vVertexSet[2] + vVertexSet[0]); 

        __subdivideTriangle({ vVertexSet[0], Middle01, Middle20 }, vDepth - 1);
        __subdivideTriangle({ vVertexSet[1], Middle12, Middle01 }, vDepth - 1);
        __subdivideTriangle({ vVertexSet[2], Middle20, Middle12 }, vDepth - 1);
        __subdivideTriangle({ Middle01, Middle12, Middle20 }, vDepth - 1);
    }
}

void CRendererTest::__updateUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 Up = glm::normalize(m_pCamera->getUp());

    m_Pipeline.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
}
