#pragma once
#include "ScenePass.h"
#include "Common.h"
#include "FrameBuffer.h"
#include "Scene.h"
#include "Camera.h"
#include "PipelineSkybox.h"
#include "PipelineSimple.h"
#include "Descriptor.h"
#include "Command.h"
#include "Image.h"
#include "Buffer.h"

#include <vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <array>
#include <set>

struct SSimplePipelineSet
{
    CPipelineSimple Main;
    CPipelineSkybox Sky;

    void destroy()
    {
        Main.destroy();
        Sky.destroy();
    }
};

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
    virtual CRenderPassPort _getPortV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _loadSceneV(ptr<SScene> vScene) override;

private:
    void __createRenderPass();
    void __createGraphicsPipelines();
    void __createCommandPoolAndBuffers();
    void __createDepthResources();
    void __createFramebuffers();
    void __createTextureImages();
    void __createVertexBuffer();
    void __createIndexBuffer();

    void __createRecreateResources();
    void __destroyRecreateResources();
    void __createSceneResources();
    void __destroySceneResources();

    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    bool __isObjectInSight(ptr<C3DObject> vpObject, const SFrustum& vFrustum) const;

    void __recordSkyRenderCommand(uint32_t vImageIndex);

    VkFormat __findDepthFormat();
    size_t __getActualTextureNum();
    void __updateDescriptorSets();

    std::vector<SSimplePointData> __readPointData(ptr<C3DObjectGoldSrc> vpObject) const;

    SSimplePipelineSet m_PipelineSet = SSimplePipelineSet();
    CCommand m_Command = CCommand();
    std::string m_SceneCommandName = "Scene";
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;

    ptr<vk::CBuffer> m_pVertexBuffer;
    ptr<vk::CBuffer> m_pIndexBuffer;
    std::vector<vk::CImage::Ptr> m_TextureImageSet;
    vk::CImage::Ptr m_pDepthImage;

    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    std::vector<SObjectDataPosition> m_ObjectDataPositions;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
};