#pragma once
#include "IPipeline.h"
#include "Common.h"
#include "VertexAttributeDescriptor.h"
#include "Descriptor.h"
#include "IOImage.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <array>

class CPipelineSkybox : public IPipeline
{
public:
    void setSkyBoxImage(const std::array<ptr<CIOImage>, 6>& vSkyBoxImageSet);
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex)
    {
        if (m_VertexBuffer.isValid())
        {
            VkBuffer VertBuffer = m_VertexBuffer;
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
    virtual void _destroyV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;

private:
    struct SPointData
    {
        glm::vec3 Pos;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            return Descriptor.generate();
        }
    };

    void __updateDescriptorSet();

    vk::CSampler m_Sampler;
    vk::CImage m_SkyBoxImage;
    vk::CBuffer m_VertexBuffer;
    size_t m_VertexNum = 0;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;
};

