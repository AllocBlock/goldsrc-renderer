#pragma once
#include "IPipeline.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Camera.h"
#include "Collider.h"

#include <map>
#include <glm/glm.hpp>

class CPipelineVisCollider : public IPipeline
{
public:
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void startRecord(VkCommandBuffer vCommandBuffer, size_t vImageIndex);
    void draw(ICollider::CPtr vCollider);
    void endRecord();

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/visColliderShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/visColliderShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _destroyV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV() override;
    virtual VkPipelineRasterizationStateCreateInfo _getRasterizationStageInfoV() override; // TIPS: counter-clockwise!

private:
    struct SVertexDataPos
    {
        uint32_t First;
        uint32_t Count;
    };

    void __updateDescriptorSet();
    void __initVertexBuffer();

    std::map<EBasicColliderType, SVertexDataPos> m_TypeVertexDataPosMap;
    vk::CBuffer m_VertexBuffer;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;

    VkCommandBuffer m_CurCommandBuffer = VK_NULL_HANDLE;
};