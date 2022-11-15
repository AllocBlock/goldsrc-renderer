#pragma once
#include "RenderPassSingle.h"
#include "FrameBuffer.h"
#include "BoundingBox.h"
#include "Camera.h"
#include "Pipeline3DGui.h"

#include <vulkan/vulkan.h> 
#include <glm/glm.hpp>

class CLineRenderPass : public CRenderPassSingle
{
public:
    CLineRenderPass() = default;

    _DEFINE_GETTER_SETTER_POINTER(Camera, CCamera::CPtr);

    void setHighlightBoundingBox(SAABB vBoundingBox);
    void removeHighlightBoundingBox();
    void addGuiLine(std::string vName, glm::vec3 vStart, glm::vec3 vEnd);

protected:
    virtual void _initV() override;
    virtual SPortDescriptor _getPortDescV() override;
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
            m_pPortSet->getInputPort("Depth")->getImageV(),
        };
    }
    virtual std::vector<VkClearValue> _getClearValuesV() override
    {
        return DefaultClearValueColorDepth;
    }

private:
    void __rerecordCommand();

    CPipelineLine m_PipelineLine;

    size_t m_RerecordCommandTimes = 0;
    CCamera::CPtr m_pCamera = nullptr;
};