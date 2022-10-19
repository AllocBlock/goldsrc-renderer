#pragma once
#include "IPipeline.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Camera.h"
#include "Collider.h"
#include "Ticker.h"

class CPipelineVisCollidePoint : public IPipeline
{
public:
    CPipelineVisCollidePoint() { m_Ticker.start(); }

    void addCollidePoint(glm::vec3 vPos, glm::vec3 vNormal, float vShowTime = 5.0f);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void record(VkCommandBuffer vCommandBuffer, size_t vImageIndex, glm::vec3 vEyePos);

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/visCollidePointShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/visCollidePointShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _destroyV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual std::vector<VkPushConstantRange> _getPushConstantRangeSetV() override;

private:
    struct SCollidePointInfo
    {
        glm::vec3 Pos;
        glm::vec3 Normal;
        float LeftTime = 0.0f;
    };

    void __updateDescriptorSet();
    void __initVertexBuffer();

    uint32_t m_VertexNum = 0;
    vk::CBuffer m_VertexBuffer;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;

    CTicker<float> m_Ticker;
    std::vector<SCollidePointInfo> m_CollidePointInfoSet;
};