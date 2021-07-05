#pragma once
#include "RendererBase.h"
#include "Common.h"
#include "Scene.h"
#include "Camera.h"
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

struct SObjectDataPosition
{
    VkDeviceSize Offset;
    VkDeviceSize Size;
};

struct SPipelineSet
{
    CPipelineDepthTest TrianglesWithDepthTest;
    CPipelineBlend TrianglesWithBlend;
    CPipelineSkybox TrianglesSky;
    CPipelineLine GuiLines;

    void destroy()
    {
        TrianglesWithDepthTest.destroy();
        TrianglesWithBlend.destroy();
        TrianglesSky.destroy();
        GuiLines.destroy();
    }
};

class CVulkanRenderer : public CRendererBase
{
public:
    CVulkanRenderer();
    
    std::shared_ptr<SScene> getScene() const { return m_pScene; }
    void loadScene(std::shared_ptr<SScene> vpScene);
    void rerecordCommand();
    std::shared_ptr<CCamera> getCamera();

    void setHighlightBoundingBox(S3DBoundingBox vBoundingBox);
    void removeHighlightBoundingBox();
    void addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd); 

    bool getSkyState() const { return m_EnableSky; }
    void setSkyState(bool vSkyState) { m_EnableSky = vSkyState && m_pScene && m_pScene->UseSkyBox; }
    bool getCullingState() const { return m_EnableCulling; }
    void setCullingState(bool vCullingState) { m_EnableCulling = vCullingState; }
    bool getFrustumCullingState() const { return m_EnableFrustumCulling; }
    void setFrustumCullingState(bool vFrustumCullingState) { m_EnableFrustumCulling = vFrustumCullingState; }
    bool getPVSState() const { return m_EnablePVS; }
    void setPVSState(bool vPVS) { m_EnablePVS = vPVS; }
    std::optional<uint32_t> getCameraNodeIndex() const { return m_CameraNodeIndex; }
    std::set<uint32_t> getRenderedNodeList() const { return m_RenderNodeSet; }
    ERenderMethod getRenderMethod() const { return m_RenderMethod; }
    std::set<size_t> getRenderedObjectSet() const { return m_RenderedObjectSet; }
    void setRenderMethod(ERenderMethod vRenderMethod) { m_RenderMethod = vRenderMethod; }

protected:
    virtual void _initV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual VkCommandBuffer _requestCommandBufferV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

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
    void __updateAllUniformBuffer(uint32_t vImageIndex);
    void __recordGuiCommandBuffer(uint32_t vImageIndex);
    void __calculateVisiableObjects();
    void __recordObjectRenderCommand(uint32_t vImageIndex, size_t vObjectIndex);
    bool __isObjectInSight(std::shared_ptr<S3DObject> vpObject, const SFrustum& vFrustum) const;
    std::pair<std::vector<size_t>, std::vector<size_t>> __sortModelRenderSequence();

    void __recordSkyRenderCommand(uint32_t vImageIndex);
    
    VkFormat __findDepthFormat();
    VkFormat __findSupportedFormat(const std::vector<VkFormat>& vCandidates, VkImageTiling vTiling, VkFormatFeatureFlags vFeatures);
    uint32_t __findMemoryType(uint32_t vTypeFilter, VkMemoryPropertyFlags vProperties);
    void __transitionImageLayout(VkImage vImage, VkFormat vFormat, VkImageLayout vOldLayout, VkImageLayout vNewLayout, uint32_t vLayerCount = 1);
    size_t __getActualTextureNum();
    void __createImageFromIOImage(std::shared_ptr<CIOImage> vpImage, Vulkan::SImagePack& voImagePack);
    void __updateDescriptorSets();

    std::vector<SGoldSrcPointData> __readPointData(std::shared_ptr<S3DObject> vpObject) const;

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

    std::shared_ptr<SScene> m_pScene;
    std::shared_ptr<CCamera> m_pCamera = nullptr;

    size_t m_RerecordCommandTimes = 0;
    std::vector<bool> m_AreObjectsVisable;
    std::set<size_t> m_RenderedObjectSet;
    std::vector<SObjectDataPosition> m_ObjectDataPositions;
    bool m_EnableSky = true;
    bool m_EnableCulling = false;
    bool m_EnableFrustumCulling = false;
    bool m_EnablePVS = false;
    std::optional<uint32_t> m_CameraNodeIndex = std::nullopt;
    std::set<uint32_t> m_RenderNodeSet;
    ERenderMethod m_RenderMethod = ERenderMethod::BSP;

    size_t m_NumSwapchainImage = 0;
};