#include "PassTest.h"
#include "ImageUtils.h"

sptr<CPortSet> CRenderPassSprite::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addOutput("Main", { VK_FORMAT_B8G8R8A8_UNORM, {0, 0}, 1, EImageUsage::COLOR_ATTACHMENT });
    PortDesc.addOutput("Depth", { VK_FORMAT_D24_UNORM_S8_UINT, {0, 0}, 1, EImageUsage::DEPTH_ATTACHMENT });
    return make<CPortSet>(PortDesc);
}

void CRenderPassSprite::_initV()
{
    m_pCamera->setFov(90);
    m_pCamera->setAspect(m_ScreenExtent.width, m_ScreenExtent.height);
    m_pCamera->setPos(glm::vec3(0.0, -1.0, 0.0));

    __loadSkyBox();
    __createVertexBuffer();

    m_pMainImage = make<vk::CImage>();
    ImageUtils::createImage2d(*m_pMainImage, m_pDevice, m_ScreenExtent, m_pPortSet->getOutputPort("Main")->getInfo().Format, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
    m_pPortSet->setOutput("Main", m_pMainImage);
    
    m_pDepthImage = make<vk::CImage>();
    ImageUtils::createDepthImage(*m_pDepthImage, m_pDevice, m_ScreenExtent);
    m_pPortSet->setOutput("Depth", m_pDepthImage);

    m_RenderInfoDescriptor.addColorAttachment(m_pPortSet->getOutputPort("Main"));
    m_RenderInfoDescriptor.setDepthAttachment(m_pPortSet->getOutputPort("Depth"));

    m_Pipeline.create(m_pDevice, m_RenderInfoDescriptor, m_ScreenExtent);
    //m_Pipeline.setImageNum(m_pAppInfo->getImageNum());
    m_Pipeline.setSkyBoxImage(m_SkyBoxImageSet);
}

void CRenderPassSprite::_updateV()
{
    m_pCamera->setAspect(m_ScreenExtent.width, m_ScreenExtent.height);

    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 Up = glm::normalize(m_pCamera->getUp());

    m_Pipeline.updateUniformBuffer(Model, View, Proj, EyePos);
}

std::vector<VkCommandBuffer> CRenderPassSprite::_requestCommandBuffersV()
{
    auto pCommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName);

    _beginCommand(pCommandBuffer);
    _beginRendering(pCommandBuffer, m_RenderInfoDescriptor.generateRendererInfo(m_ScreenExtent));

    if (m_pVertexBuffer->isValid())
    {
        pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
        m_Pipeline.bind(pCommandBuffer);


        uint32_t VertexNum = static_cast<uint32_t>(m_PointDataSet.size());
        pCommandBuffer->draw(0, VertexNum);
    }
    
    _endRendering();
    _endCommand();
    return { pCommandBuffer->get()};
}

void CRenderPassSprite::_destroyV()
{
    destroyAndClear(m_pDepthImage);
    m_Pipeline.destroy();

    m_pVertexBuffer->destroy();
    m_pVertexBuffer = nullptr;

    IRenderPass::_destroyV();
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
