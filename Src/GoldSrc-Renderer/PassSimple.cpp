#include "PassSimple.h"
#include "Common.h"
#include "Descriptor.h"
#include "Function.h"
#include "Gui.h"
#include "RenderPassDescriptor.h"

#include <iostream>
#include <array>
#include <vector>
#include <set>
#include <fstream>
#include <chrono>
#include <glm/ext/matrix_transform.hpp>

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

    __createCommandPoolAndBuffers();
    __createRecreateResources();

    m_pPortSet->getOutputPort("Main")->hookImageUpdate([=] { m_NeedUpdateFramebuffer = true; rerecordCommand(); });
    m_pPortSet->getOutputPort("Depth")->hookImageUpdate([=] { m_NeedUpdateFramebuffer = true; rerecordCommand(); });

    rerecordCommand();
}

SPortDescriptor CSceneSimpleRenderPass::_getPortDescV()
{
    SPortDescriptor Ports;
    Ports.addInputOutput("Main");
    
    VkFormat DepthFormat = __findDepthFormat();
    Ports.addOutput("Depth", { DepthFormat, m_AppInfo.Extent, 1 });
    return Ports;
}

CRenderPassDescriptor CSceneSimpleRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"), 
                                                            m_pPortSet->getOutputPort("Depth"));
}

void CSceneSimpleRenderPass::_recreateV()
{
    IRenderPass::_recreateV();

    __destroyRecreateResources();
    __createRecreateResources();
    rerecordCommand();
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
    __destroyRecreateResources();

    m_Command.clear();

    IRenderPass::_destroyV();
}

std::vector<VkCommandBuffer> CSceneSimpleRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    if (m_NeedUpdateFramebuffer || m_IsUpdated)
    {
        __createFramebuffers();
        m_NeedUpdateFramebuffer = false;
    }

    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);

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

// FIXME: better solution exist
void CSceneSimpleRenderPass::_onRenderPassRecreateV()
{
    __createGraphicsPipelines(); // extent
    m_PipelineSet.Main.setImageNum(m_AppInfo.ImageNum);
    m_PipelineSet.Sky.setImageNum(m_AppInfo.ImageNum);
}

void CSceneSimpleRenderPass::__createRecreateResources()
{
    __createDepthResources(); // extent
    if (isValid())
    {
        __createGraphicsPipelines(); // extent
        m_PipelineSet.Main.setImageNum(m_AppInfo.ImageNum);
        m_PipelineSet.Sky.setImageNum(m_AppInfo.ImageNum);
    }
    __createSceneResources();
}

void CSceneSimpleRenderPass::__destroyRecreateResources()
{
    m_pDepthImage->destroy();

    for (auto& pFramebuffer : m_FramebufferSet)
        pFramebuffer->destroy();
    m_FramebufferSet.clear();

    __destroySceneResources();
    m_PipelineSet.destroy();
}

void CSceneSimpleRenderPass::__createSceneResources()
{
    __createTextureImages();
    __updateDescriptorSets();
    __createVertexBuffer();

    m_EnableSky = m_EnableSky && m_pScene && m_pScene->UseSkyBox;

    if (m_pScene && m_pScene->UseSkyBox)
    {
        m_PipelineSet.Sky.setSkyBoxImage(m_pScene->SkyBoxImages);
    }
}

void CSceneSimpleRenderPass::__destroySceneResources()
{
    for (size_t i = 0; i < m_TextureImageSet.size(); ++i)
    {
        m_TextureImageSet[i]->destroy();
    }
    m_TextureImageSet.clear();

    if (m_pIndexBuffer) m_pIndexBuffer->destroy();
    if (m_pVertexBuffer) m_pVertexBuffer->destroy();
}

void CSceneSimpleRenderPass::__recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex)
{
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    _recordObjectRenderCommand(CommandBuffer, vObjectIndex);
}

void CSceneSimpleRenderPass::__createGraphicsPipelines()
{
    m_PipelineSet.Sky.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
    m_PipelineSet.Main.create(m_AppInfo.pDevice, get(), m_AppInfo.Extent);
}

void CSceneSimpleRenderPass::__createCommandPoolAndBuffers()
{
    m_Command.createPool(m_AppInfo.pDevice, ECommandType::RESETTABLE);
    m_Command.createBuffers(m_SceneCommandName, static_cast<uint32_t>(m_AppInfo.ImageNum), ECommandBufferLevel::PRIMARY);
}

void CSceneSimpleRenderPass::__createDepthResources()
{
    VkFormat DepthFormat = m_pPortSet->getOutputFormat("Depth").Format;
    m_pDepthImage = Function::createDepthImage(m_AppInfo.pDevice, m_AppInfo.Extent, NULL, DepthFormat);
    m_pPortSet->setOutput("Depth", m_pDepthImage);
}

void CSceneSimpleRenderPass::__createFramebuffers()
{
    _ASSERTE(isValid());

    m_FramebufferSet.resize(m_AppInfo.ImageNum);
    for (size_t i = 0; i < m_AppInfo.ImageNum; ++i)
    {
        std::vector<VkImageView> AttachmentSet =
        {
            m_pPortSet->getOutputPort("Main")->getImageV(i),
            *m_pDepthImage
        };

        m_FramebufferSet[i] = make<vk::CFrameBuffer>();
        m_FramebufferSet[i]->create(m_AppInfo.pDevice, get(), AttachmentSet, m_AppInfo.Extent);
    }
}

void CSceneSimpleRenderPass::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageSet.resize(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
            m_TextureImageSet[i] = Function::createImageFromIOImage(m_AppInfo.pDevice, m_pScene->TexImageSet[i]);
        }
    }
}

void CSceneSimpleRenderPass::__createVertexBuffer()
{
    size_t NumVertex = 0;
    if (m_pScene)
    {
        for (ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
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
    for (ptr<C3DObjectGoldSrc> pObject : m_pScene->Objects)
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
    std::vector<VkImageView> TextureSet(m_TextureImageSet.size());
    for (size_t i = 0; i < m_TextureImageSet.size(); ++i)
        TextureSet[i] = *m_TextureImageSet[i];
    m_PipelineSet.Main.updateDescriptorSet(TextureSet);
}

std::vector<SSimplePointData> CSceneSimpleRenderPass::__readPointData(ptr<C3DObjectGoldSrc> vpObject) const
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

VkFormat CSceneSimpleRenderPass::__findDepthFormat()
{
    return m_AppInfo.pDevice->getPhysicalDevice()->chooseSupportedFormat(
        { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
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
    VkCommandBuffer CommandBuffer = m_Command.getCommandBuffer(m_SceneCommandName, vImageIndex);
    m_PipelineSet.Sky.recordCommand(CommandBuffer, vImageIndex);
}