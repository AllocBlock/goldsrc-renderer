#include "PassSimple.h"
#include "Function.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"
#include "Log.h"

#include <vector>
#include <fstream>

void CSceneSimpleRenderPass::_loadSceneV(ptr<SScene> vScene)
{
    m_AppInfo.pDevice->waitUntilIdle();
    m_pScene = vScene;

    m_AreObjectsVisable.clear();
    m_AreObjectsVisable.resize(m_pScene->Objects.size(), false);

    __destroySceneResources();
    __createSceneResources();

    rerecordCommand();
}

void CSceneSimpleRenderPass::rerecordCommand()
{
    m_RerecordCommandTimes += m_AppInfo.ImageNum;
}

void CSceneSimpleRenderPass::_initV()
{
    IRenderPass::_initV();

    rerecordCommand();
}

SPortDescriptor CSceneSimpleRenderPass::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    
    VkFormat DepthFormat = m_AppInfo.pDevice->getPhysicalDevice()->getBestDepthFormat();
    Ports.addOutput("Depth", { DepthFormat, m_AppInfo.Extent, 1, EUsage::WRITE });
    return Ports;
}

CRenderPassDescriptor CSceneSimpleRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"), 
                                                            m_pPortSet->getOutputPort("Depth"));
}

void CSceneSimpleRenderPass::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    if (vUpdateState.RenderpassUpdated || vUpdateState.ImageExtent.IsUpdated || vUpdateState.ImageNum.IsUpdated)
    {
        __createDepthResources(); // extent
        if (isValid())
        {
            __createGraphicsPipelines(); // extent
            m_PipelineSet.Main.setImageNum(m_AppInfo.ImageNum);
            m_PipelineSet.Sky.setImageNum(m_AppInfo.ImageNum);

            if (m_pScene && m_pScene->UseSkyBox)
            {
                m_PipelineSet.Sky.setSkyBoxImage(m_pScene->SkyBoxImages);
            }
        }
        __createFramebuffers();
        rerecordCommand();
    }
}

void CSceneSimpleRenderPass::_updateV(uint32_t vImageIndex)
{
    __updateAllUniformBuffer(vImageIndex);
}

void CSceneSimpleRenderPass::_renderUIV()
{
    if (UI::collapse(u8"渲染器设置"))
    {
        bool SkyRendering = getSkyState();
        UI::toggle(u8"开启天空渲染", SkyRendering);
        setSkyState(SkyRendering);

        bool Culling = getCullingState();
        UI::toggle(u8"开启剔除", Culling);
        setCullingState(Culling);
        Culling = getCullingState();

        if (Culling)
        {
            UI::indent(20.0f);
            bool FrustumCulling = getFrustumCullingState();
            UI::toggle(u8"CPU视锥剔除", FrustumCulling);
            setFrustumCullingState(FrustumCulling);
            UI::unindent();
        }
    }
}

void CSceneSimpleRenderPass::_destroyV()
{
    m_DepthImage.destroy();

    m_FramebufferSet.destroyAndClearAll();

    m_PipelineSet.destroy();

    __destroySceneResources();
    CSceneRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CSceneSimpleRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);

    bool RerecordCommand = false;
    if (m_EnableCulling || m_RerecordCommandTimes > 0)
    {
        RerecordCommand = true;
        if (m_RerecordCommandTimes > 0) --m_RerecordCommandTimes;
    }
    if (RerecordCommand)
    {
        // init
        std::vector<VkClearValue> ClearValueSet(2);
        ClearValueSet[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        ClearValueSet[1].depthStencil = { 1.0f, 0 };

        begin(CommandBuffer, *m_FramebufferSet[vImageIndex], m_AppInfo.Extent, ClearValueSet);

        if (m_EnableSky)
            __recordSkyRenderCommand(vImageIndex);
        
        bool Valid = true;
        VkDeviceSize Offsets[] = { 0 };
        if (m_pVertexBuffer &&m_pVertexBuffer->isValid())
        {
            VkBuffer VertBuffer = *m_pVertexBuffer;
            vkCmdBindVertexBuffers(CommandBuffer, 0, 1, &VertBuffer, Offsets);
        }
        else if (m_pIndexBuffer && m_pIndexBuffer->isValid())
            vkCmdBindIndexBuffer(CommandBuffer, *m_pIndexBuffer, 0, VK_INDEX_TYPE_UINT32);
        else
            Valid = false;

        if (Valid)
        {
            __calculateVisiableObjects();
            m_PipelineSet.Main.bind(CommandBuffer, vImageIndex);
            for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
            {
                if (m_AreObjectsVisable[i])
                {
                    __recordObjectRenderCommand(vImageIndex, i);
                }
            }
        }

        end();
    }
    return { CommandBuffer };
}

void CSceneSimpleRenderPass::__createSceneResources()
{
    __createTextureImages();
    __updateDescriptorSets();
    __createVertexBuffer();

    m_EnableSky = m_EnableSky && m_pScene && m_pScene->UseSkyBox;

    if (m_pScene && m_pScene->UseSkyBox && m_PipelineSet.Sky.isValid())
    {
        m_PipelineSet.Sky.setSkyBoxImage(m_pScene->SkyBoxImages);
    }
}

void CSceneSimpleRenderPass::__destroySceneResources()
{
    m_TextureImageSet.destroyAndClearAll();

    if (m_pIndexBuffer) m_pIndexBuffer->destroy();
    if (m_pVertexBuffer) m_pVertexBuffer->destroy();
}

void CSceneSimpleRenderPass::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);
    _recordObjectRenderCommand(CommandBuffer, vObjectIndex);
}

void CSceneSimpleRenderPass::__createGraphicsPipelines()
{
    m_PipelineSet.Sky.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
    m_PipelineSet.Main.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CSceneSimpleRenderPass::__createDepthResources()
{
    m_DepthImage.destroy();

    VkFormat DepthFormat = m_pPortSet->getOutputFormat("Depth").Format;
    Function::createDepthImage(m_DepthImage, m_AppInfo.pDevice, m_AppInfo.Extent, NULL, DepthFormat);
    m_pPortSet->setOutput("Depth", m_DepthImage);
}

void CSceneSimpleRenderPass::__createFramebuffers()
{
    if (!isValid()) return;

    m_FramebufferSet.destroyAndClearAll();

    m_FramebufferSet.init(m_AppInfo.ImageNum);
    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
            m_DepthImage
        };

        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CSceneSimpleRenderPass::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageSet.init(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
             Function::createImageFromIOImage(*m_TextureImageSet[i], m_AppInfo.pDevice, m_pScene->TexImageSet[i]);
        }
    }
}

void CSceneSimpleRenderPass::__createVertexBuffer()
{
    size_t NumVertex = 0;
    if (m_pScene)
    {
        for (ptr<CMeshDataGoldSrc> pObject : m_pScene->Objects)
            NumVertex += pObject->getVertexArray()->size();
        if (NumVertex == 0)
        {
            Common::Log::log(u8"没有顶点数据，跳过顶点缓存创建");
            return;
        } 
    }
    else
        return;

    VkDeviceSize BufferSize = sizeof(SSimplePointData) * NumVertex;
    uint8_t* pData = new uint8_t[BufferSize];
    size_t Offset = 0;
    for (ptr<CMeshDataGoldSrc> pObject : m_pScene->Objects)
    {
        std::vector<SSimplePointData> PointData = __readPointData(pObject);
        size_t SubBufferSize = sizeof(SSimplePointData) * pObject->getVertexArray()->size();
        memcpy(reinterpret_cast<char*>(pData) + Offset, PointData.data(), SubBufferSize);
        Offset += SubBufferSize;
    }

    m_pVertexBuffer = make<vk::CBuffer>();
    m_pVertexBuffer->create(m_AppInfo.pDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pVertexBuffer->stageFill(pData, BufferSize);
    delete[] pData;
}

void CSceneSimpleRenderPass::__updateDescriptorSets()
{
    m_PipelineSet.Main.updateDescriptorSet(m_TextureImageSet);
}

std::vector<SSimplePointData> CSceneSimpleRenderPass::__readPointData(ptr<CMeshDataGoldSrc> vpObject) const
{
    auto pVertexArray = vpObject->getVertexArray();
    auto pNormalArray = vpObject->getNormalArray();
    auto pTexCoordArray = vpObject->getTexCoordArray();
    auto pTexIndexArray = vpObject->getTexIndexArray();

    size_t NumPoint = pVertexArray->size();
    _ASSERTE(NumPoint == pNormalArray->size());
    _ASSERTE(NumPoint == pTexCoordArray->size());
    _ASSERTE(NumPoint == pTexIndexArray->size());

    std::vector<SSimplePointData> PointData(NumPoint);
    for (size_t i = 0; i < NumPoint; ++i)
    {
        PointData[i].Pos = pVertexArray->get(i);
        PointData[i].Normal = pNormalArray->get(i);
        PointData[i].TexCoord = pTexCoordArray->get(i);
        PointData[i].TexIndex = pTexIndexArray->get(i);
    }
    return PointData;
}

size_t CSceneSimpleRenderPass::__getActualTextureNum()
{
    size_t NumTexture = m_pScene ? m_pScene->TexImageSet.size() : 0;
    if (NumTexture > CPipelineSimple::MaxTextureNum)
    {
        Common::Log::log(u8"警告: 纹理数量 = (" + std::to_string(NumTexture) + u8") 大于限制数量 (" + std::to_string(CPipelineSimple::MaxTextureNum) + u8"), 多出的纹理将被忽略");
        NumTexture = CPipelineSimple::MaxTextureNum;
    }
    return NumTexture;
}

void CSceneSimpleRenderPass::__calculateVisiableObjects()
{
    if (!m_pScene) return;

    SFrustum Frustum = m_pCamera->getFrustum();

    for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
    {
        m_AreObjectsVisable[i] = false;

        if (m_pScene->Objects[i]->getMark() == "sky" || m_pScene->Objects[i]->getVertexArray()->empty())
            continue;

        if (m_EnableCulling)
        {
            if (i >= m_pScene->BspTree.NodeNum + m_pScene->BspTree.LeafNum) // ignore culling for model for now
            {
                m_AreObjectsVisable[i] = true;
                continue;
            }

            // frustum culling: don't draw object outside of view (judge by bounding box)
            if (m_EnableFrustumCulling)
                if (!_isObjectInSight(m_pScene->Objects[i], Frustum))
                    continue;
        }

        m_AreObjectsVisable[i] = true;
    }
}

void CSceneSimpleRenderPass::__updateAllUniformBuffer(uint32_t vImageIndex)
{
    float Aspect = 1.0;
    if (m_AppInfo.Extent.height > 0 && m_AppInfo.Extent.width > 0)
        Aspect = static_cast<float>(m_AppInfo.Extent.width) / m_AppInfo.Extent.height;
    m_pCamera->setAspect(Aspect);

    glm::mat4 Model = glm::mat4(1.0f);
    m_PipelineSet.Main.updateUniformBuffer(vImageIndex, Model, m_pCamera);
    if (m_EnableSky)
        m_PipelineSet.Sky.updateUniformBuffer(vImageIndex, m_pCamera);
}

void CSceneSimpleRenderPass::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_DefaultCommandName, vImageIndex);
    m_PipelineSet.Sky.recordCommand(CommandBuffer, vImageIndex);
}