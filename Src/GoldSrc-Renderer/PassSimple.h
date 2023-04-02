#pragma once
#include "PointData.h"
#include "PassScene.h"
#include "FrameBuffer.h"
#include "SceneInfoGoldSrc.h"
#include "Camera.h"
#include "Image.h"
#include "PipelineSkybox.h"
#include "PipelineSimple.h"
#include "DynamicResourceManager.h"

class CSceneSimpleRenderPass : public CRenderPassSceneTyped<SSimplePointData>
{
public:
    CSceneSimpleRenderPass() = default;

    void rerecordCommand();

    bool getSkyState() const { return m_EnableSky; }
    void setSkyState(bool vSkyState)
    {
        bool EnableSky = vSkyState && m_pSceneInfo && m_pSceneInfo->UseSkyBox;
        if (EnableSky != m_EnableSky)
        {
            rerecordCommand();
            m_EnableSky = EnableSky;
        }
    }
    bool getCullingState() const { return m_EnableCulling; }
    void setCullingState(bool vCullingState) { m_EnableCulling = vCullingState; }
    bool getFrustumCullingState() const { return m_EnableFrustumCulling; }
    void setFrustumCullingState(bool vFrustumCullingState) { m_EnableFrustumCulling = vFrustumCullingState; }

protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

    virtual bool _dumpReferenceExtentV(VkExtent2D& voExtent) override
    {
        return _dumpInputPortExtent("Main", voExtent);
    }
    virtual std::vector<VkImageView> _getAttachmentsV(uint32_t vIndex) override
    {
        return
        {
            m_pPortSet->getOutputPort("Main")->getImageV(vIndex),
            m_pPortSet->getOutputPort("Depth")->getImageV(),
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColorDepth;
    }

    virtual void _loadSceneV(ptr<SSceneInfoGoldSrc> vScene) override;

private:
    void __createTextureImages();

    void __createSceneResources();
    void __destroySceneResources();

    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __drawActor(uint32_t vImageIndex, CActor::Ptr vActor);

    void __recordSkyRenderCommand(uint32_t vImageIndex);
    
    size_t __getActualTextureNum();

    struct
    {
        CDynamicPipeline<CPipelineSimple> Main;
        CDynamicPipeline<CPipelineSkybox> Sky;

        void destroy()
        {
            Main.destroy();
            Sky.destroy();
        }
    } m_PipelineSet;

    vk::CPointerSet<vk::CImage> m_TextureImageSet;
    CDynamicTextureCreator m_DepthImageManager;

    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
};
