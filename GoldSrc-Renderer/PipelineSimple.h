#pragma once
#include "PipelineBase.h"
#include "PointData.h"
#include "Image.h"
#include "Buffer.h"

#include <glm/glm.hpp>

class CPipelineSimple : public CPipelineBase
{
public:
    void updateDescriptorSet(const std::vector<VkImageView>& vTextureSet);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos);
    void destroy();

    static size_t MaxTextureNum; // if need change, you should change this in frag shader as well

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shader/simpleShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shader/simpleShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
private:
    void __destroyResources();
    
    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    std::vector<std::shared_ptr<vk::CBuffer>> m_VertUniformBufferSet;
    std::vector<std::shared_ptr<vk::CBuffer>> m_FragUniformBufferSet;
    std::shared_ptr<vk::CImage> m_pPlaceholderImage;
};