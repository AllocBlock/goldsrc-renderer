#include "PipelineShade.h"

struct SUBOVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
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
    UBOVert.View = vCamera->getViewMat();
    UBOVert.Proj = vCamera->getProjMat();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);
}

void CPipelineShade::updatePushConstant(VkCommandBuffer vCommandBuffer, const glm::mat4& vModelMatrix)
{
    SPushConstant Constant = { vModelMatrix, glm::transpose(glm::inverse(vModelMatrix)) };
    pushConstant(vCommandBuffer, VK_SHADER_STAGE_VERTEX_BIT, Constant);
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

std::vector<VkPushConstantRange> CPipelineShade::_getPushConstantRangeSetV()
{
    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    return { PushConstantInfo };
}

VkPipelineRasterizationStateCreateInfo CPipelineShade::_getRasterizationStageInfoV()
{
    auto StageInfo = IPipeline::getDefaultRasterizationStageInfo();
    StageInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    return StageInfo;
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
