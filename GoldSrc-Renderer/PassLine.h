#pragma once
#include "ScenePass.h"
#include "Common.h"
#include "FrameBuffer.h"
#include "Scene.h"
#include "Camera.h"
#include "Descriptor.h"
#include "Command.h"
#include "Image.h"
#include "Buffer.h"
#include "PipelineLine.h"

#include <vulkan/vulkan.h> 
#include <glm/glm.hpp>
#include <array>
#include <set>

class CLineRenderPass : public CSceneRenderPass
{
public:
    CLineRenderPass() = default;

    void setHighlightBoundingBox(S3DBoundingBox vBoundingBox);
    void removeHighlightBoundingBox();
    void addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd);

protected:
    virtual void _initV() override;
    virtual CRenderPassPort _getPortV() override;
    virtual void _recreateV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
    virtual void _renderUIV() override;
    virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) override;
    virtual void _destroyV() override;

    virtual void _loadSceneV(ptr<SScene> vScene) override {}

private:
    void __rerecordCommand();
    void __createRenderPass();
    void __createCommandPoolAndBuffers();
    void __createFramebuffers();

    void __createRecreateResources();
    void __destroyRecreateResources();

    CPipelineLine m_PipelineLine;
    CCommand m_Command = CCommand();
    std::string m_CommandName = "Gui";
    std::vector<ptr<vk::CFrameBuffer>> m_FramebufferSet;

    size_t m_RerecordCommandTimes = 0;
};