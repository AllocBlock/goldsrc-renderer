#include "PassPBR.h"
#include "..\..\Gui\InterfaceGui.h"
#include "ImageUtils.h"
#include "RenderPassDescriptor.h"

void CRenderPassPBR::_initV()
{
    CRenderPassSingleFrameBuffer::_initV();
    
    VkExtent2D RefExtent = { 0, 0 };
    _dumpReferenceExtentV(RefExtent);

    m_DepthImageManager.init(RefExtent, false,
        [this](VkExtent2D vExtent, vk::CPointerSet<vk::CImage>& vImageSet)
        {
            vImageSet.init(1);
            VkFormat DepthFormat = m_pPortSet->getOutputPortInfo("Depth").Format;
            ImageUtils::createDepthImage(*vImageSet[0], m_pDevice, vExtent, NULL, DepthFormat);
            m_pPortSet->setOutput("Depth", *vImageSet[0]);
        }
    );

    m_PipelineCreator.init(m_pDevice, weak_from_this(), RefExtent, false, m_pAppInfo->getImageNum(), [this](CPipelinePBS& vPipeline)
    {
        vPipeline.setMaterialBuffer(m_pMaterialBuffer);
        vPipeline.setTextures(m_TextureColorSet, m_TextureNormalSet, m_TextureSpecularSet);

        sptr<CIOImage> pSkyIOImage = make<CIOImage>("./textures/old_hall_4k.exr");
        pSkyIOImage->read();
        sptr<CIOImage> pSkyIrrIOImage = make<CIOImage>("./textures/old_hall_4k_irr.exr");
        pSkyIrrIOImage->read();
        vPipeline.setSkyTexture(pSkyIOImage, pSkyIrrIOImage);
    });

    __createVertexBuffer();
    __createMaterials();
}

sptr<CPortSet> CRenderPassPBR::_createPortSetV()
{
    SPortDescriptor PortDesc;
    PortDesc.addInputOutput("Main", SPortInfo::createAnyOfUsage(EImageUsage::COLOR_ATTACHMENT));
    VkFormat DepthFormat = VK_FORMAT_D24_UNORM_S8_UINT;
    PortDesc.addOutput("Depth", { DepthFormat, {0, 0}, 1, EImageUsage::DEPTH_ATTACHMENT });
    return make<CPortSet>(PortDesc);
}

CRenderPassDescriptor CRenderPassPBR::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"),
        m_pPortSet->getOutputPort("Depth"));
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
        UI::indent();
        UI::inputColor("Base Color", m_PipelineControl.Material.Albedo);
        UI::drag("Metallic", m_PipelineControl.Material.OMR.g, 0.01f, 0.01f, 0.99f);
        UI::drag("Roughness", m_PipelineControl.Material.OMR.b, 0.01f, 0.01f, 0.99f);
        UI::unindent();
    }
}

std::vector<VkCommandBuffer> CRenderPassPBR::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (!m_PipelineCreator.isReady())
        throw "Not Ready";

    sptr<CCommandBuffer> pCommandBuffer = _getCommandBuffer(vImageIndex);

    _beginWithFramebuffer(vImageIndex);

    if (m_pVertexBuffer->isValid())
    {
        pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
        m_PipelineCreator.get().bind(pCommandBuffer, vImageIndex);
        pCommandBuffer->draw(0, m_pVertexBuffer->getVertexNum());
    }
    
    _endWithFramebuffer();
    return { pCommandBuffer->get() };
}

void CRenderPassPBR::_destroyV()
{
    m_PipelineCreator.destroy();
    m_DepthImageManager.destroy();
    m_TextureColorSet.destroyAndClearAll();
    m_TextureNormalSet.destroyAndClearAll();
    m_TextureSpecularSet.destroyAndClearAll();
    destroyAndClear(m_pMaterialBuffer);
    destroyAndClear(m_pVertexBuffer);
    
    CRenderPassSingleFrameBuffer::_destroyV();
}

void CRenderPassPBR::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    m_DepthImageManager.updateV(vUpdateState);
    m_PipelineCreator.updateV(vUpdateState);

    VkExtent2D RefExtent = { 0, 0 };
    if (_dumpReferenceExtentV(RefExtent))
    {
        if (m_pCamera)
            m_pCamera->setAspect(RefExtent.width, RefExtent.height);

        m_DepthImageManager.updateExtent(RefExtent);
        m_PipelineCreator.updateExtent(RefExtent);
    }
    
    CRenderPassSingleFrameBuffer::_onUpdateV(vUpdateState);
}

void CRenderPassPBR::__createVertexBuffer()
{
     __generateScene();
    size_t VertexNum = m_PointDataSet.size();

    if (!m_PointDataSet.empty())
    {
        auto pVertBuffer = make<vk::CVertexBufferTyped<CPipelinePBS::SPointData>>();
        pVertBuffer->create(m_pDevice, m_PointDataSet);
        m_pVertexBuffer = pVertBuffer;
    }
}

void CRenderPassPBR::__createMaterials()
{
    m_TextureColorSet.init(1);
    sptr<CIOImage> pColorImage = make<CIOImage>("./textures/Stone_albedo.jpg");
    pColorImage->read();
    ImageUtils::createImageFromIOImage(*m_TextureColorSet[0], m_pDevice, pColorImage);

    m_TextureNormalSet.init(1);
    sptr<CIOImage> pNormalImage = make<CIOImage>("./textures/Stone_normal.jpg");
    pNormalImage->read();
    ImageUtils::createImageFromIOImage(*m_TextureNormalSet[0], m_pDevice, pNormalImage);

    m_TextureSpecularSet.init(1);
    sptr<CIOImage> pSpecularImage = make<CIOImage>("./textures/Stone_omr.jpg");
    pSpecularImage->read();
    vk::CImage Specular;
    ImageUtils::createImageFromIOImage(*m_TextureSpecularSet[0], m_pDevice, pSpecularImage);

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
    m_pMaterialBuffer->create(m_pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pMaterialBuffer->stageFill(MaterialSet.data(), BufferSize);
}

void CRenderPassPBR::__generateScene()
{
    const float Sqrt2 = static_cast<float>(std::sqrt(2));
    const float Sqrt6 = static_cast<float>(std::sqrt(6));
    const float OneThrid = 1.0f / 3.0f;
    std::array<glm::vec3, 4> VertexSet =
    {
        glm::vec3(              0.0,       1.0,                  0.0),
        glm::vec3(              0.0, -OneThrid, 2 * OneThrid * Sqrt2),
        glm::vec3( OneThrid * Sqrt6, -OneThrid,    -OneThrid * Sqrt2),
        glm::vec3(-OneThrid * Sqrt6, -OneThrid,    -OneThrid * Sqrt2),
    };

    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
        {
            uint32_t Index = uint32_t(i * 8 + j);
            glm::vec3 Center = glm::vec3((i - 3.5f) * 3, (j - 3.5f) * 3, 0);

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
            float u = std::atan2(Vertex.z, Vertex.x) * 0.5f / glm::pi<float>() + 0.5f;
            float v = Vertex.y * 0.5f + 0.5f;
            glm::vec2 TexCoord = glm::vec2(u, v);

            glm::vec3 Normal = glm::normalize(Vertex);
            glm::vec3 Bitangent = cross(Normal, glm::vec3(0.0f, 0.0f, 1.0f));
            glm::vec3 Tangent = cross(Bitangent, Normal);

            m_PointDataSet.emplace_back(CPipelinePBS::SPointData({ vCenter + Vertex, Normal, Tangent, TexCoord, vMaterialIndex }));
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
    m_pCamera->setAspect(m_pAppInfo->getScreenAspect());

    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 View = m_pCamera->getViewMat();
    glm::mat4 Proj = m_pCamera->getProjMat();
    glm::vec3 EyePos = m_pCamera->getPos();
    glm::vec3 Up = glm::normalize(m_pCamera->getUp());

    m_PipelineCreator.get().updateUniformBuffer(vImageIndex, Model, View, Proj, EyePos, m_PipelineControl);
}
