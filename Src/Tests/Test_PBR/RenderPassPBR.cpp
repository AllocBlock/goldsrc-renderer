#include "RenderPassPBR.h"
#include "Gui.h"
#include "Function.h"
#include "RenderPassDescriptor.h"

void CRenderPassPBR::_initV()
{
    __createRenderPass();
    __createVertexBuffer();
    __createRecreateResources();
}

SPortDescriptor CRenderPassPBR::_getPortDescV()
{
    CRenderPassPort Ports;
    Ports.addInput("Main", m_AppInfo.ImageFormat, m_AppInfo.Extent);
    Ports.addOutput("Main", m_AppInfo.ImageFormat, m_AppInfo.Extent);
    return Ports;
}

void CRenderPassPBR::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
}

void CRenderPassPBR::_updateV(uint32_t vImageIndex)
{
    __updateUniformBuffer(vImageIndex);
}

void CRenderPassPBR::_renderUIV()
{
    UI::toggle("Use Color Texture", m_PipelineControl.UseColorTexture);
    UI::toggle("Use Normal Texture", m_PipelineControl.UseNormalTexture);
    UI::toggle("Use Specular Texture", m_PipelineControl.UseSpecularTexture);

    UI::split();

    UI::toggle("Force Use Material", m_PipelineControl.ForceUseMat);
    if (m_PipelineControl.ForceUseMat)
    {
        UI::indent(20.0f);
        UI::inputColor("Base Color", m_PipelineControl.Material.Albedo);
        UI::drag("Metallic", m_PipelineControl.Material.OMR.g, 0.01f, 0.01f, 0.99f);
        UI::drag("Roughness", m_PipelineControl.Material.OMR.b, 0.01f, 0.01f, 0.99f);
        UI::unindent();
    }
}

std::vector<VkCommandBuffer> CRenderPassPBR::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (!m_Pipeline.isReady())
        throw "Not Ready";

    if (m_FramebufferSet.empty())
        __createFramebuffers();

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

void CRenderPassPBR::_destroyV()
{
    __destroyRecreateResources();
    m_pVertexBuffer->destroy();
    m_pVertexBuffer = nullptr;

    IRenderPass::_destroyV();
}

void CRenderPassPBR::__createRenderPass()
{
    CRenderPassDescriptor Desc;
    Desc.addColorAttachment(m_RenderPassPosBitField, m_AppInfo.ImageFormat);
    Desc.setDepthAttachment(vk::ERenderPassPos::BEGIN, VK_FORMAT_D32_SFLOAT);
    auto Info = Desc.generateInfo();
    vk::checkError(vkCreateRenderPass(*m_AppInfo.pDevice, &Info, nullptr, _getPtr()));
}

void CRenderPassPBR::__createGraphicsPipeline()
{
    m_Pipeline.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CRenderPassPBR::__createDepthResources()
{
    m_pDepthImage = Function::createDepthImage(m_AppInfo.pDevice, m_AppInfo.Extent);
}

void CRenderPassPBR::__createFramebuffers()
{
    _ASSERTE(isValid());

    size_t ImageNum = m_AppInfo.ImageNum;
    m_FramebufferSet.resize(ImageNum);
    for (size_t i = 0; i < ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pLink->getOutput("Main", i),
            *m_pDepthImage
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CRenderPassPBR::__createVertexBuffer()
{
     __generateScene();
    size_t VertexNum = m_PointDataSet.size();

    if (VertexNum > 0)
    {
        VkDeviceSize BufferSize = sizeof(SPBSPointData) * VertexNum;
        m_pVertexBuffer = make<vk::CBuffer>();
        m_pVertexBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_pVertexBuffer->stageFill(m_PointDataSet.data(), BufferSize);
    }
}

void CRenderPassPBR::__createMaterials()
{ 
    CIOImage::Ptr pColorImage = make<CIOImage>("./textures/Stone_albedo.jpg");
    pColorImage->read();
    vk::CImage::Ptr pColor = Function::createImageFromIOImage(m_AppInfo.pDevice, pColorImage);
    m_TextureColorSet.push_back(pColor);

    CIOImage::Ptr pNormalImage = make<CIOImage>("./textures/Stone_normal.jpg");
    pNormalImage->read();
    vk::CImage::Ptr pNormal = Function::createImageFromIOImage(m_AppInfo.pDevice, pNormalImage);
    m_TextureNormalSet.push_back(pNormal);

    CIOImage::Ptr pSpecularImage = make<CIOImage>("./textures/Stone_omr.jpg");
    pSpecularImage->read();
    vk::CImage::Ptr pSpecular = Function::createImageFromIOImage(m_AppInfo.pDevice, pSpecularImage);
    m_TextureSpecularSet.push_back(pSpecular);

    _ASSERTE(m_GridSize > 0);
    size_t Num = m_GridSize * m_GridSize;

    std::vector<SMaterialPBR> MaterialSet(Num);

    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
        {
            int Index = i * 8 + j;
            MaterialSet[Index].Albedo = glm::vec4(1.0f);
            MaterialSet[Index].OMR.g = float(i) / 7.0f;
            MaterialSet[Index].OMR.b = float(j) / 7.0f;
            MaterialSet[Index].ColorIdx = 0;
            MaterialSet[Index].NormalIdx = 0;
            MaterialSet[Index].SpecularIdx = 0;
        }

    VkDeviceSize BufferSize = sizeof(SMaterialPBR) * Num;
    m_pMaterialBuffer = make<vk::CBuffer>();
    m_pMaterialBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pMaterialBuffer->stageFill(MaterialSet.data(), BufferSize);
}

void CRenderPassPBR::__createRecreateResources()
{
    __createMaterials();

    __createGraphicsPipeline(); 
    __createDepthResources();
    m_Pipeline.setImageNum(m_AppInfo.ImageNum);
    m_Pipeline.setMaterialBuffer(m_pMaterialBuffer);
    m_Pipeline.setTextures(m_TextureColorSet, m_TextureNormalSet, m_TextureSpecularSet);

    CIOImage::Ptr pSkyIOImage = make<CIOImage>("./textures/old_hall_4k.exr");
    pSkyIOImage->read();
    CIOImage::Ptr pSkyIrrIOImage = make<CIOImage>("./textures/old_hall_4k_irr.exr");
    pSkyIrrIOImage->read();
    m_Pipeline.setSkyTexture(pSkyIOImage, pSkyIrrIOImage);
}

void CRenderPassPBR::__destroyRecreateResources()
{
    m_pDepthImage->destroy();

    for (int i = 0; i < m_TextureColorSet.size(); ++i)
        m_TextureColorSet[i]->destroy();
    m_TextureColorSet.clear();
    for (int i = 0; i < m_TextureNormalSet.size(); ++i)
        m_TextureNormalSet[i]->destroy();
    m_TextureNormalSet.clear();
    for (int i = 0; i < m_TextureSpecularSet.size(); ++i)
        m_TextureSpecularSet[i]->destroy();
    m_TextureSpecularSet.clear();

    m_pMaterialBuffer->destroy();

    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();
    m_Pipeline.destroy();
}

void CRenderPassPBR::__generateScene()
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

    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
        {
            uint32_t Index = uint32_t(i * 8 + j);
            glm::vec3 Center = glm::vec3((i - 3.5f) * 3, 0, (j - 3.5f) * 3);

            __subdivideTriangle({ VertexSet[0], VertexSet[1], VertexSet[2] }, Center, Index, 4);
            __subdivideTriangle({ VertexSet[0], VertexSet[2], VertexSet[3] }, Center, Index, 4);
            __subdivideTriangle({ VertexSet[0], VertexSet[3], VertexSet[1] }, Center, Index, 4);
            __subdivideTriangle({ VertexSet[3], VertexSet[2], VertexSet[1] }, Center, Index, 4);
        }
    
}

void CRenderPassPBR::__subdivideTriangle(std::array<glm::vec3, 3> vVertexSet, glm::vec3 vCenter, uint32_t vMaterialIndex, int vDepth)
{
    if (vDepth == 0)
    {
        for (const auto& Vertex : vVertexSet)
        {
            float u = std::atan2(Vertex.y, Vertex.x) * 0.5 / glm::pi<float>() + 0.5f;
            float v = Vertex.z * 0.5f + 0.5f;
            glm::vec2 TexCoord = glm::vec2(u, v);

            glm::vec3 Normal = glm::normalize(Vertex);
            glm::vec3 Bitangent = cross(Normal, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::vec3 Tangent = cross(Bitangent, Normal);

            m_PointDataSet.emplace_back(SPBSPointData({ vCenter + Vertex, Normal, Tangent, TexCoord, vMaterialIndex }));
        }
    }
    else
    {
        glm::vec3 Middle01 = glm::normalize(vVertexSet[0] + vVertexSet[1]);
        glm::vec3 Middle12 = glm::normalize(vVertexSet[1] + vVertexSet[2]);
        glm::vec3 Middle20 = glm::normalize(vVertexSet[2] + vVertexSet[0]); 

        __subdivideTriangle({ vVertexSet[0], Middle01, Middle20 }, vCenter, vMaterialIndex, vDepth - 1);
        __subdivideTriangle({ vVertexSet[1], Middle12, Middle01 }, vCenter, vMaterialIndex, vDepth - 1);
        __subdivideTriangle({ vVertexSet[2], Middle20, Middle12 }, vCenter, vMaterialIndex, vDepth - 1);
        __subdivideTriangle({ Middle01, Middle12, Middle20 }, vCenter, vMaterialIndex, vDepth - 1);
    }
}

void CRenderPassPBR::__updateUniformBuffer(uint32_t vImageIndex)
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

    m_Pipeline.updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos, m_PipelineControl);
}
