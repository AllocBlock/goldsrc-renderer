#include "PipelineShade.h"

struct SUBOVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Model;
};

void CPipelineShade::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        m_Descriptor.update(i, WriteInfo);
    }
}

void CPipelineShade::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Model = glm::mat4(1.0f);
    UBOVert.View = vCamera->getViewMat();
    UBOVert.Proj = vCamera->getProjMat();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineShade::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SPointData::getBindingDescription();
    voAttributeSet = SPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineShade::_getInputAssemblyStageInfoV()
{
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

void CPipelineShade::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_VertUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
    }

    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );

    __updateDescriptorSet();
}

void CPipelineShade::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineShade::_destroyV()
{
    __destroyResources();
}

void CPipelineShade::__destroyResources()
{
    m_VertUniformBufferSet.destroyAndClearAll();
}
