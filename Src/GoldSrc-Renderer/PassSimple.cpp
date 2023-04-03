#include "PassSimple.h"
#include "Function.h"
#include "InterfaceUI.h"
#include "RenderPassDescriptor.h"
#include "Log.h"

#include <vector>
#include <fstream>

void CSceneSimpleRenderPass::_loadSceneV(ptr<SSceneInfoGoldSrc> vScene)
{
    CRenderPassSceneTyped::_loadSceneV(vScene);

    m_pDevice->waitUntilIdle();

    m_AreObjectsVisable.clear();
    m_AreObjectsVisable.resize(m_pSceneInfo->pScene->getActorNum(), false);

    __destroySceneResources();
    __createSceneResources();

    rerecordCommand();
}

void CSceneSimpleRenderPass::rerecordCommand()
{
    m_RerecordCommandTimes = m_pAppInfo->getImageNum();
}

void CSceneSimpleRenderPass::_initV()
{
    CRenderPassSceneTyped::_initV();

    VkExtent2D RefExtent = { 0, 0 };
    _dumpReferenceExtentV(RefExtent);

    m_DepthImageManager.init(RefExtent, false,
        [this](VkExtent2D vExtent, vk::CPointerSet<vk::CImage>& vImageSet)
        {
            vImageSet.init(1);
            VkFormat DepthFormat = m_pPortSet->getOutputFormat("Depth").Format;
            Function::createDepthImage(*vImageSet[0], m_pDevice, vExtent, NULL, DepthFormat);
            m_pPortSet->setOutput("Depth", *vImageSet[0]);
        }
    );

    VkExtent2D ScreenExtent = m_pAppInfo->getScreenExtent();

    m_PipelineSet.Main.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), [this](CPipelineSimple& vPipeline)
    {
        if (vPipeline.isValid())
            vPipeline.setTextures(m_TextureImageSet);
        rerecordCommand();
    });
    m_PipelineSet.Sky.init(m_pDevice, weak_from_this(), ScreenExtent, true, m_pAppInfo->getImageNum(), [this](CPipelineSkybox& vPipeline)
    {
        if (vPipeline.isValid() && m_pSceneInfo && m_pSceneInfo->UseSkyBox)
        {
            vPipeline.setSkyBoxImage(m_pSceneInfo->SkyBoxImages);
        }
        rerecordCommand();
    });

    rerecordCommand();
}

void CSceneSimpleRenderPass::_initPortDescV(SPortDescriptor& vioDesc)
{
    vioDesc.addInputOutput("Main", SPortFormat::createAnyOfUsage(EUsage::WRITE));
    
    VkFormat DepthFormat = m_pDevice->getPhysicalDevice()->getBestDepthFormat();
    vioDesc.addOutput("Depth", { DepthFormat, {0, 0}, 1, EUsage::WRITE });
}

CRenderPassDescriptor CSceneSimpleRenderPass::_getRenderPassDescV()
{
    return CRenderPassDescriptor::generateSingleSubpassDesc(m_pPortSet->getOutputPort("Main"), 
                                                            m_pPortSet->getOutputPort("Depth"));
}

void CSceneSimpleRenderPass::_onUpdateV(const vk::SPassUpdateState& vUpdateState)
{
    m_DepthImageManager.updateV(vUpdateState);
    m_PipelineSet.Main.updateV(vUpdateState);
    m_PipelineSet.Sky.updateV(vUpdateState);



    VkExtent2D RefExtent = { 0, 0 };
    if (_dumpReferenceExtentV(RefExtent))
    {
        if (m_pCamera)
            m_pCamera->setAspect(RefExtent.width, RefExtent.height);
        m_DepthImageManager.updateExtent(RefExtent);
    }

    CRenderPassSingle::_onUpdateV(vUpdateState);
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
    m_DepthImageManager.destroy();
    m_PipelineSet.destroy();

    __destroySceneResources();
    CRenderPassSceneTyped::_destroyV();
}

std::vector<VkCommandBuffer> CSceneSimpleRenderPass::_requestCommandBuffersV(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);

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

        _beginWithFramebuffer(vImageIndex);

        if (m_EnableSky)
            __recordSkyRenderCommand(vImageIndex);
        
        bool Valid = true;
        VkDeviceSize Offsets[] = { 0 };
        if (isNonEmptyAndValid(m_pVertexBuffer))
        {
            pCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
        }
        else
            Valid = false;

        if (Valid)
        {
            __calculateVisiableObjects();
            m_PipelineSet.Main.get().bind(pCommandBuffer, vImageIndex);
            for (size_t i = 0; i < m_pSceneInfo->pScene->getActorNum(); ++i)
            {
                if (m_AreObjectsVisable[i])
                {
                    __drawActor(vImageIndex, m_pSceneInfo->pScene->getActor(i));
                }
            }
        }

        _endWithFramebuffer();
    }
    return { pCommandBuffer->get() };
}

void CSceneSimpleRenderPass::__createSceneResources()
{
    __createTextureImages();

    m_EnableSky = m_EnableSky && m_pSceneInfo && m_pSceneInfo->UseSkyBox;

    if (m_PipelineSet.Main.isReady())
        m_PipelineSet.Main.get().setTextures(m_TextureImageSet);

    if (m_pSceneInfo && m_pSceneInfo->UseSkyBox && m_PipelineSet.Sky.isReady())
    {
        m_PipelineSet.Sky.get().setSkyBoxImage(m_pSceneInfo->SkyBoxImages);
    }
}

void CSceneSimpleRenderPass::__destroySceneResources()
{
    m_TextureImageSet.destroyAndClearAll();
}

void CSceneSimpleRenderPass::__drawActor(uint32_t vImageIndex, CActor::Ptr vActor)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
    _drawActor(pCommandBuffer, vActor);
}

void CSceneSimpleRenderPass::__createTextureImages()
{
    size_t NumTexture = __getActualTextureNum();
    if (NumTexture > 0)
    {
        m_TextureImageSet.init(NumTexture);
        for (size_t i = 0; i < NumTexture; ++i)
        {
             Function::createImageFromIOImage(*m_TextureImageSet[i], m_pDevice, m_pSceneInfo->TexImageSet[i]);
        }
    }
}

size_t CSceneSimpleRenderPass::__getActualTextureNum()
{
    size_t NumTexture = m_pSceneInfo ? m_pSceneInfo->TexImageSet.size() : 0;
    if (NumTexture > CPipelineSimple::MaxTextureNum)
    {
        Log::log("警告: 纹理数量 = (" + std::to_string(NumTexture) + ") 大于限制数量 (" + std::to_string(CPipelineSimple::MaxTextureNum) + "), 多出的纹理将被忽略");
        NumTexture = CPipelineSimple::MaxTextureNum;
    }
    return NumTexture;
}

void CSceneSimpleRenderPass::__calculateVisiableObjects()
{
    if (!m_pSceneInfo) return;

    auto pScene = m_pSceneInfo->pScene;

    SFrustum Frustum = m_pCamera->getFrustum();

    for (size_t i = 0; i < pScene->getActorNum(); ++i)
    {
        auto pActor = pScene->getActor(i);

        m_AreObjectsVisable[i] = false;

        if (pActor->hasTag("sky") || m_ActorSegmentMap.find(pActor) == m_ActorSegmentMap.end())
            continue;

        if (m_EnableCulling)
        {
            if (i >= m_pSceneInfo->BspTree.NodeNum + m_pSceneInfo->BspTree.LeafNum) // ignore culling for model for now
            {
                m_AreObjectsVisable[i] = true;
                continue;
            }

            // frustum culling: don't draw object outside of view (judge by bounding box)
            if (m_EnableFrustumCulling)
                if (!isActorInSight(pActor, Frustum))
                    continue;
        }

        m_AreObjectsVisable[i] = true;
    }
}

void CSceneSimpleRenderPass::__updateAllUniformBuffer(uint32_t vImageIndex)
{
    glm::mat4 Model = glm::mat4(1.0f);
    m_PipelineSet.Main.get().updateUniformBuffer(vImageIndex, Model, m_pCamera);
    if (m_EnableSky)
        m_PipelineSet.Sky.get().updateUniformBuffer(vImageIndex, m_pCamera);
}

void CSceneSimpleRenderPass::__recordSkyRenderCommand(uint32_t vImageIndex)
{
    CCommandBuffer::Ptr pCommandBuffer = _getCommandBuffer(vImageIndex);
    m_PipelineSet.Sky.get().recordCommand(pCommandBuffer, vImageIndex);
}
