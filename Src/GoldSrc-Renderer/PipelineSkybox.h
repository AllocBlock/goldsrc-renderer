#pragma once
#include "Pipeline.h"
#include "Common.h"
#include "VertexAttributeDescriptor.h"
#include "Descriptor.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"

#include <glm/glm.hpp>
#include <array>

class CPipelineSkybox : public IPipeline
{
public:
    void destroy();
    void setSkyBoxImage(const std::array<ptr<CIOImage>, 6>& vSkyBoxImageSet);
    void updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, glm::vec3 vUp);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
    {
        if (m_pVertexBuffer->isValid())
        {
            VkBuffer VertBuffer = *m_pVertexBuffer;
            const VkDeviceSize Offsets[] = { 0 };
            bind(vCommandBuffer, vImageIndex);
            vkCmdBindVertexBuffers(vCommandBuffer, 0, 1, &VertBuffer, Offsets);
            vkCmdDraw(vCommandBuffer, static_cast<uint32_t>(m_VertexNum), 1, 0, 0);
        }
    }

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/skyShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/skyShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;

private:
    struct SPointData
    {
        glm::vec3 Pos;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription BindingDescription = {};
            BindingDescription.binding = 0;
            BindingDescription.stride = sizeof(SPointData);
            BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return BindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SPointData, Pos));
            return Descriptor.generate();
        }
    };

    void __updateDescriptorSet();

    vk::CSampler m_Sampler;
    vk::CImage::Ptr m_pSkyBoxImage; // cubemap
    ptr<vk::CBuffer> m_pVertexBuffer;
    size_t m_VertexNum = 0;
    std::vector<ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
    std::vector<ptr<vk::CUniformBuffer>> m_FragUniformBufferSet;
};

