#pragma once
#include "Pipeline.h"
#include "VertexAttributeDescriptor.h""
#include <glm/glm.hpp>
#include "Camera.h"
#include "UniformBuffer.h"
#include "IOImage.h"

struct SFullScreenPointData
{
    glm::vec3 Pos;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription BindingDescription = {};
        BindingDescription.binding = 0;
        BindingDescription.stride = sizeof(SFullScreenPointData);
        BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return BindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
    {
        VertexAttributeDescriptor Descriptor;
        Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SFullScreenPointData, Pos));
        return Descriptor.generate();
    }
};

class CPipelineEnvironment : public IPipeline
{
public:

    void setEnvironmentMap(CIOImage::Ptr vHDRI);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::Ptr vCamera);
    void destroy();

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shader/envVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shader/envFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    void __createPlaceholderImage();
    void __updateDescriptorSet();
    void __destroyResources();

    struct SControl
    {
        glm::vec3 Rotation;
    } m_Control;

    VkSampler m_Sampler = VK_NULL_HANDLE;
    std::vector<vk::CUniformBuffer::Ptr> m_VertUBSet;
    std::vector<vk::CUniformBuffer::Ptr> m_FragUBSet;
    vk::CImage::Ptr m_pEnvironmentImage = nullptr;
    vk::CImage::Ptr m_pPlaceholderImage = nullptr;
};


