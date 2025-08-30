#include "PipelineEnvironment.h"
#include "ImageUtils.h"
#include "FullScreenPointData.h"

namespace
{
    struct SUBOFrag
    {
        glm::mat4 InverseVP;
        glm::vec4 EyePos;
    };
}

void CPipelineEnvironment::setEnvironmentMap(CIOImage::Ptr vSkyImage)
{
    ImageUtils::createImageFromIOImage(m_EnvironmentImage, m_pDevice, vSkyImage);
    __precalculateIBL(vSkyImage);
    __updateDescriptorSet();
    m_Ready = true;
}

void CPipelineEnvironment::updateUniformBuffer(uint32_t vImageIndex, CCamera::Ptr vCamera)
{
    SUBOFrag UBOVert = {};
    UBOVert.InverseVP = glm::inverse(vCamera->getViewProjMat());
    UBOVert.EyePos = glm::vec4(vCamera->getPos(), 1.0);
    m_pFragUniformBuffer->update(&UBOVert);
}

CPipelineDescriptor CPipelineEnvironment::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("env.vert");
    Descriptor.setFragShaderPath("env.frag");

    Descriptor.setVertexInputInfo<SFullScreenPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);

    return Descriptor;
}

void CPipelineEnvironment::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("CombinedSampler", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

void CPipelineEnvironment::_createV()
{
    __destroyResources();
    m_pFragUniformBuffer = make<vk::CUniformBuffer>();
    m_pFragUniformBuffer->create(m_pDevice, sizeof(SUBOFrag));

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
    ImageUtils::createPlaceholderImage(m_PlaceholderImage, m_pDevice);
}

void CPipelineEnvironment::__updateDescriptorSet()
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteBuffer(0, *m_pFragUniformBuffer);
    VkImageView EnvImageView = m_EnvironmentImage.isValid() ? m_EnvironmentImage : m_PlaceholderImage;
    WriteInfo.addWriteImageAndSampler(1, EnvImageView, m_Sampler.get());

    m_ShaderResourceDescriptor.update(WriteInfo);
}

void CPipelineEnvironment::__destroyResources()
{
    destroyAndClear(m_pFragUniformBuffer);

    m_EnvironmentImage.destroy();
    m_PlaceholderImage.destroy();

    m_Sampler.destroy();
}