#pragma once
#include "RenderPassSingle.h"
#include "FrameBuffer.h"
#include "BoundingBox.h"
#include "Camera.h"
#include "DynamicResourceManager.h"
#include "VisualizePrimitive.h"
#include "PipelineTriangle.h"
#include "PipelineLine.h"
#include "PipelinePoint.h"
#include "PipelineSphere.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

class CRenderPassVisualize : public CRenderPassSingle
{
public:
    CRenderPassVisualize() = default;

    _DEFINE_GETTER_SETTER_POINTER(Camera, CCamera::CPtr);

    void addTriangle(const Visualize::Triangle& vTriangle, const glm::vec3& vColor);
    void addTriangle(const glm::vec3& vA, const glm::vec3& vB, const glm::vec3& vC, const glm::vec3& vColor);
    void addLine(const Visualize::Line& vLine, const glm::vec3& vColor);
    void addLine(const glm::vec3& vStart, const glm::vec3& vEnd, const glm::vec3& vColor);
    void addPoint(const Visualize::Point& vPoint, const glm::vec3& vColor);
    void addSphere(const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor);
    void clearAll();

protected:
    virtual void _initV() override;
    virtual void _initPortDescV(SPortDescriptor& vioDesc) override;
    virtual CRenderPassDescriptor _getRenderPassDescV() override;
    virtual void _updateV(uint32_t vImageIndex) override;
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

    struct
    {
        CDynamicPipeline<CPipelineTriangle> Triangle;
        CDynamicPipeline<CPipelineLine> Line;
        CDynamicPipeline<CPipelinePoint> Point;
        CDynamicPipeline<CPipelineSphere> Sphere;
    } m_PipelineSet;

    size_t m_RerecordCommandTimes = 0;
    CCamera::CPtr m_pCamera = nullptr;
};