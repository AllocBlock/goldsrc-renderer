#include "PassTest.h"
#include "RenderPassDescriptor.h"
#include "Function.h"

void CRenderPassSprite::_initV()
{
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_FirstInputExtent.width, m_FirstInputExtent.height);
    m_pCamera->setPos(glm::vec3(0.0, -1.0, 0.0));

    __loadSkyBox();
    __createVertexBuffer();
    __createRecreateResources();
}

SPortDescriptor CRenderPassSprite::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    VkFormat DepthFormat = m_pDevice->getPhysicalDevice()->getBestDepthFormat();
    Ports.addOutput("Depth", { DepthFormat, {0, 0}, 1, EUsage::UNDEFINED });
    return Ports;
}

CRenderPassDescriptor CRenderPassSprite::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
        m_pPortSet->getOutputPort("Depth"));
}

void CRenderPassSprite::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

std::vector<VkCommandBuffer> CRenderPassSprite::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    std::vector<VkClearValue> ClearValueSet(2);
    ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
    ClearValueSet[1].depthStencil = { 1.0f, 0 };

    begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_FirstInputExtent, ClearValueSet);

    if (m_pVertexBuffer->isValid())
    {
        VkBuffer VertBuffer = *m_pVertexBuffer;
        VkDeviceSize Offsets[] = { 0 };
        vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        m_Pipeline.bind(CommandBuffer, vImageIndex);

        uint32_t VertexNum = static_cast<uint32_t>(m_PointDataSet.size());
        vkCmdDraw(CommandBuffer, VertexNum, 1, 0, 0);
    }
    
    end();
    return { CommandBuffer };
}

void CRenderPassSprite::_destroyV()
{
    __destroyRecreateResources();
    m_pVertexBuffer->destroy();
    m_pVertexBuffer = nullptr;

    IRenderPass::_destroyV();
}

void CRenderPassSprite::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassSprite::__loadSkyBox()
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

bool CRenderPassSprite::__readSkyboxImages(std::string vSkyFilePrefix, std::string vExtension)
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

void CRenderPassSprite::__createGraphicsPipeline()
{
    m_Pipeline.create(m_pDevice, get(), m_FirstInputExtent);
}

void CRenderPassSprite::__createDepthResources()
{
    Function::createDepthImage(m_DepthImage, m_pDevice, m_FirstInputExtent);
    m_pPortSet->setOutput("Depth", m_DepthImage);
}

void CRenderPassSprite::__createFramebuffers(VkExtent2D vExtent)
{
    _ASSERTE(isValid());

    uint32_t ImageNum = m_pAppInfo->getImageNum();
    m_FramebufferSet.init(ImageNum);
    for (uint32_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
            m_DepthImage
        };
        
        m_FramebufferSet[i]->create(m_pDevice, get(), AttachmentSet, m_FirstInputExtent);
    }
}

void CRenderPassSprite::__createVertexBuffer()
{
     __generateScene();
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(CPipelineTest::SPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}

void CRenderPassSprite::__createRecreateResources()
{
    __createDepthResources();
    if (isValid())
    {
        __createGraphicsPipeline();
        m_Pipeline.setImageNum(m_pAppInfo->getImageNum());
        m_Pipeline.setSkyBoxImage(m_SkyBoxImageSet);
        __createFramebuffers(VkExtent2D vExtent);
    }
}

void CRenderPassSprite::__destroyRecreateResources()
{
    m_DepthImage.destroy();
    m_FramebufferSet.destroyAndClearAll();
    m_Pipeline.destroy();
}

void CRenderPassSprite::__generateScene()
{
    const float Sqrt2 = static_cast<float>(std::sqrt(2));
    const float Sqrt6 = static_cast<float>(std::sqrt(6));
    const float OneThrid = 1.0f / 3.0f;
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

void CRenderPassSprite::__subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, int vDepth)
{
    if (vDepth == 0)
    {
        for (const auto& Vertex : vVertexSet)
            m_PointDataSet.emplace_back(CPipelineTest::SPointData({ Vertex, Vertex }));
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

void CRenderPassSprite::__updateUniformBuffer(uint32_t vImageIndex)
{
    m_pCamera->setAspect(m_FirstInputExtent.width, m_FirstInputExtent.height);

    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 Up = glm::normalize(m_pCamera->getUp());

    m_Pipeline.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos);
}
