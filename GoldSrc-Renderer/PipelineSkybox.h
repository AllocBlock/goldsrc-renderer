#pragma once
#include "PipelineBase.h"
#include "Common.h"
#include "PointData.h"
#include "Descriptor.h"
#include "IOImage.h"

#include <glm/glm.hpp>
#include <array>

class CPipelineSkybox : public CPipelineBase
{
public:
    void destroy();
    void setSkyBoxImage(const std::array<std::shared_ptr<CIOImage>, 6>& vSkyBoxImageSet);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, glm::vec3 vUp);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
    {
        if (m_VertexDataPack.isValid())
        {
            const VkDeviceSize Offsets[] = { 0 };
            bind(vCommandBuffer, vImageIndex);
            vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &m_VertexDataPack.Buffer, Offsets);
            vkCmdDraw(vCommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
        }
    }

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shader/skyShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shader/skyShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;

private:
    void __updateDescriptorSet();

    VkSampler m_TextureSampler = VK_NULL_HANDLE;
    Vulkan::SImagePack m_SkyBoxImagePack; // cubemap
    Vulkan::SBufferPack m_VertexDataPack;
    size_t m_VertexNum = 0;
    std::vector<Vulkan::SBufferPack> m_VertUniformBufferPacks;
    std::vector<Vulkan::SBufferPack> m_FragUniformBufferPacks;
};

