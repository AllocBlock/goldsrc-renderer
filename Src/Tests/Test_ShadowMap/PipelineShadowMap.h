#pragma once
#include "Pipeline.h"
#include "Camera.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "VertexAttributeDescriptor.h"

#include <glm/glm.hpp>
#include <array>

struct SShadowMapPointData
{
    glm::vec3 Pos;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SShadowMapPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        CVertexAttributeDescriptor Descriptor;
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SShadowMapPointData, Pos));
        return Descriptor.generate();
    }
};

class CPipelineShadowMap : public IPipeline
{
public:
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vLightViewProj, float vLightNear, float vLightFar);
    void destroy();

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/shadowMapVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/shadowMapFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;

private:
    void __updateDescriptorSet();
    void __destroyResources();

    std::vector<ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
    std::vector<ptr<vk::CUniformBuffer>> m_FragUniformBufferSet;
};

