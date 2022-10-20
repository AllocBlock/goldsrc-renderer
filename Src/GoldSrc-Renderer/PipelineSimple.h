#pragma once
#include "Pipeline.h"
#include "Image.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"

#include <glm/glm.hpp>

class CPipelineSimple : public IPipeline
{
public:
    void updateDescriptorSet(const vk::CPointerSet<vk::CImage>& vTextureSet);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, CCamera::CPtr vCamera);

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/simpleShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/simpleShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initShaderResourceDescriptorV() override;
    virtual void _destroyV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
private:
    void __destroyResources();

    vk::CSampler m_Sampler;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
    vk::CImage m_PlaceholderImage;
};