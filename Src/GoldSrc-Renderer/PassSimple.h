#pragma once
#include "PointData.h"
#include "ScenePass.h"
#include "FrameBuffer.h"
#include "Scene.h"
#include "Camera.h"
#include "PipelineSkybox.h"
#include "PipelineSimple.h"
#include "Image.h"
#include "Buffer.h"

#include <vulkan/vulkan.h>

class CSceneSimpleRenderPass : public CSceneRenderPass
{
public:
    CSceneSimpleRenderPass() = default;

    void rerecordCommand();

    bool getSkyState() const { return m_EnableSky; }
    void setSkyState(bool vSkyState) {
        bool EnableSky = vSkyState && m_pScene && m_pScene->UseSkyBox;
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
    virtual SPortDescriptor _getPortDescV() override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _onUpdateV(const vk::SPassUpdateState& vUpdateState) override;

    virtual void _loadSceneV(ptr<SScene> vScene) override;

private:
    void __createGraphicsPipelines();
    void __createDepthResources();
    void __createFramebuffers();
    void __createTextureImages();
    void __createVertexBuffer();

    void __createSceneResources();
    void __destroySceneResources();

    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);

    void __recordSkyRenderCommand(uint32_t vImageIndex);

    VkFormat __findDepthFormat();
    size_t __getActualTextureNum();
    void __updateDescriptorSets();

    std::vector<SSimplePointData> __readPointData(ptr<C3DObjectGoldSrc> vpObject) const;

    struct
    {
        CPipelineSimple Main;
        CPipelineSkybox Sky;

        void destroy()
        {
            Main.destroy();
            Sky.destroy();
        }
    } m_PipelineSet;

    vk::CPointerSet<vk::CFrameBuffer> m_FramebufferSet;

    ptr<vk::CBuffer> m_pVertexBuffer;
    ptr<vk::CBuffer> m_pIndexBuffer;
    vk::CPointerSet<vk::CImage> m_TextureImageSet;
    vk::CImage m_DepthImage;

    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
};