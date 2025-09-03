#include "PipelineShade.h"

namespace
{
    struct SUBOVert
    {
        alignas(16) glm::mat4 Proj;
        alignas(16) glm::mat4 View;
    };
}

void CPipelineShade::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineShade::updateUniformBuffer(uint32_t vImageIndex, cptr<CCamera> vCamera)
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

void CPipelineShade::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineShade::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("shaders/shaderVert.spv");
    Descriptor.setFragShaderPath("shaders/shaderFrag.spv");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
    Descriptor.addPushConstant<SPushConstant>(VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT);
    Descriptor.setRasterCullMode(VkCullModeFlagBits::VK_CULL_MODE_NONE);
    Descriptor.setRasterFrontFace(VkFrontFace::VK_FRONT_FACE_COUNTER_CLOCKWISE);

    return Descriptor;
}

void CPipelineShade::_createV(size_t vImageNum)
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

void CPipelineShade::_initPushConstantV(VkCommandBuffer vCommandBuffer)
{
    updatePushConstant(vCommandBuffer, glm::identity<glm::mat4>());
}

void CPipelineShade::_destroyV()
{
    __destroyResources();
}

void CPipelineShade::__destroyResources()
{
    m_VertUniformBufferSet.destroyAndClearAll();
}
