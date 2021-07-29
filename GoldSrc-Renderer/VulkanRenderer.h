#pragma once
#include "RendererScene.h"
#include "Common.h"
#include "PipelineSkybox.h"
#include "PipelineDepthTest.h"
#include "PipelineBlend.h"
#include "PipelineLine.h"
#include "Descriptor.h"
#include "Command.h"

#include <vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <array>
#include <optional>
#include <set>

enum class ERenderMethod
{
    DEFAULT,
    BSP
};

struct SPipelineSet
{
    CPipelineDepthTest Main;
    CPipelineBlend TrianglesWithBlend;
    CPipelineSkybox Sky;
    CPipelineLine GuiLines;

    void destroy()
    {
        Main.destroy();
        TrianglesWithBlend.destroy();
        Sky.destroy();
        GuiLines.destroy();
    }
};

class CRendererSceneGoldSrc : public CRendererScene
{
public:
    CRendererSceneGoldSrc() = default;
    
    void rerecordCommand();

    void setHighlightBoundingBox(S3DBoundingBox vBoundingBox);
    void removeHighlightBoundingBox();
    void addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd); 

    bool getSkyState() const { return m_EnableSky; }
    bool getCullingState() const { return m_EnableCulling; }
    bool getFrustumCullingState() const { return m_EnableFrustumCulling; }
    bool getPVSState() const { return m_EnablePVS; }
    bool getBSPState() const { return m_EnableBSP; }
    std::optional<uint32_t> getCameraNodeIndex() const { return m_CameraNodeIndex; }
    std::set<uint32_t> getRenderedNodeList() const { return m_RenderNodeSet; }
    std::set<size_t> getRenderedObjectSet() const { return m_RenderedObjectSet; }

    void setSkyState(bool vEnableSky)
    { 
        bool EnableSky = vEnableSky && m_pScene && m_pScene->UseSkyBox;
        if (m_EnableSky != EnableSky) rerecordCommand();
        m_EnableSky = EnableSky;
    }
    void setCullingState(bool vEnableCulling) 
    { 
        if (m_EnableCulling != vEnableCulling) rerecordCommand();
        m_EnableCulling = vEnableCulling;
    }
    void setFrustumCullingState(bool vEnableFrustumCulling) 
    { 
        if (m_EnableFrustumCulling != vEnableFrustumCulling) rerecordCommand();
        m_EnableFrustumCulling = vEnableFrustumCulling;
    }
    void setPVSState(bool vPVS) 
    { 
        bool EnablePVS = vPVS && m_pScene && m_pScene->BspPvs.LeafNum > 0;
        if (m_EnablePVS != EnablePVS) rerecordCommand();
        m_EnablePVS = EnablePVS;
    }
    void setBSPState(bool vEnableBSP) 
    { 
        bool EnableBSP = vEnableBSP && m_pScene && !m_pScene->BspTree.Nodes.empty();
        if (!EnableBSP)
            m_RenderNodeSet.clear();
        if (m_EnableBSP != EnableBSP) rerecordCommand();
        m_EnableBSP = EnableBSP;
    }

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _loadSceneV(std::shared_ptr<SScene> vScene);

private:
    void __createRenderPass();
    void __destroyRenderPass();
    void __createGraphicsPipelines();
    void __createCommandPoolAndBuffers();
    void __createDepthResources();
    void __createFramebuffers();
    void __createTextureImages();
    void __createLightmapImage();
    void __createVertexBuffer();
    void __createIndexBuffer();
    
    void __createRecreateResources();
    void __destroyRecreateResources();
    void __createSceneResources();
    void __destroySceneResources();

    void __renderByBspTree(uint32_t vImageIndex);
    void __renderTreeNode(uint32_t vImageIndex, uint32_t vNodeIndex);
    void __renderModels(uint32_t vImageIndex);
    void __renderModel(uint32_t vImageIndex, size_t vModelIndex, bool vBlend);
    void __renderPointEntities(uint32_t vImageIndex);
    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __recordGuiCommandBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    bool __isObjectInSight(std::shared_ptr<C3DObject> vpObject, const SFrustum& vFrustum) const;
    std::pair<std::vector<size_t>, std::vector<size_t>> __sortModelRenderSequence();

    void __recordSkyRenderCommand(uint32_t vImageIndex);
    
    VkFormat __findDepthFormat();
    VkFormat __findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures);
    uint32_t __findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties);
    void __transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount = 1);
    size_t __getActualTextureNum();
    void __createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, Vulkan::SImagePack& voImagePack);
    void __updateDescriptorSets();

    std::vector<SGoldSrcPointData> __readPointData(std::shared_ptr<C3DObjectGoldSrc> vpObject) const;

    VkRenderPass m_RenderPass = VK_NULL_HANDLE;
    SPipelineSet m_PipelineSet = SPipelineSet();
    CCommand m_Command = CCommand();
    std::string m_SceneCommandName = "Scene";
    std::string m_GuiCommandName = "Gui";
    std::vector<VkFramebuffer> m_FramebufferSet;

    Vulkan::SBufferPack m_VertexBufferPack;
    Vulkan::SBufferPack m_IndexBufferPack;
    std::vector<Vulkan::SImagePack> m_TextureImagePackSet;
    Vulkan::SImagePack m_DepthImagePack;
    Vulkan::SImagePack m_LightmapImagePack;

    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    std::set<size_t> m_RenderedObjectSet;
    std::vector<SObjectDataPosition> m_ObjectDataPositions;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
    bool m_EnablePVS = false;
    bool m_EnableBSP = false;
    std::optional<uint32_t> m_CameraNodeIndex = std::nullopt;
    std::set<uint32_t> m_RenderNodeSet;

    size_t m_NumSwapchainImage = 0;
};