#include "PipelineEnvironment.h"
#include "Function.h"
#include "FullScreenPointData.h"

namespace
{
    struct SUBOVert
    {
        glm::mat4 InverseVP;
        glm::vec4 EyePos;
    };
}

void CPipelineEnvironment::setEnvironmentMap(CIOImage::Ptr vSkyImage)
{
    Function::createImageFromIOImage(m_EnvironmentImage, m_pDevice, vSkyImage);
    __precalculateIBL(vSkyImage);
    __updateDescriptorSet();
    m_Ready = true;
}

void CPipelineEnvironment::updateUniformBuffer(uint32_t vImageIndex, CCamera::Ptr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.InverseVP = glm::inverse(vCamera->getViewProjMat());
    UBOVert.EyePos = glm::vec4(vCamera->getPos(), 1.0);
    m_FragUBSet[vImageIndex]->update(&UBOVert);
}

CPipelineDescriptor CPipelineEnvironment::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("shaders/envVert.spv");
    Descriptor.setFragShaderPath("shaders/envFrag.spv");

    Descriptor.setVertexInputInfo<SFullScreenPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);

    return Descriptor;
}

void CPipelineEnvironment::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("CombinedSampler", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

void CPipelineEnvironment::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_FragUBSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_FragUBSet[i]->create(m_pDevice, VertBufferSize);
    }

    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    __createPlaceholderImage();
}

void CPipelineEnvironment::_destroyV()
{
    __destroyResources();
}

void CPipelineEnvironment::__precalculateIBL(CIOImage::Ptr vSkyImage)
{
    // TODO: implement IBL 
}

void CPipelineEnvironment::__createPlaceholderImage()
{
    Function::createPlaceholderImage(m_PlaceholderImage, m_pDevice);
}

void CPipelineEnvironment::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_FragUBSet[i]);
        VkImageView EnvImageView = m_EnvironmentImage.isValid() ? m_EnvironmentImage : m_PlaceholderImage;
        WriteInfo.addWriteImageAndSampler(1, EnvImageView, m_Sampler.get());

        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineEnvironment::__destroyResources()
{
    m_FragUBSet.destroyAndClearAll();

    m_EnvironmentImage.destroy();
    m_PlaceholderImage.destroy();

    m_Sampler.destroy();
}