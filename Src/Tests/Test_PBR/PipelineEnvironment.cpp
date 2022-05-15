#include "PipelineEnvironment.h"
#include "Function.h"

struct SUBOVert
{
    glm::mat4 InverseVP;
    glm::vec4 EyePos;
}; 

void CPipelineEnvironment::setEnvironmentMap(CIOImage::Ptr vSkyImage)
{
    m_pEnvironmentImage = Function::createImageFromIOImage(m_pDevice, vSkyImage);
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

void CPipelineEnvironment::destroy()
{
    __destroyResources();
    IPipeline::destroy();
}

void CPipelineEnvironment::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SFullScreenPointData::getBindingDescription();
    voAttributeSet = SFullScreenPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineEnvironment::_getInputAssemblyStageInfoV()
{
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

void CPipelineEnvironment::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    m_FragUBSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_FragUBSet[i] = make<vk::CUniformBuffer>();
        m_FragUBSet[i]->create(m_pDevice, VertBufferSize);
    }

    const auto& Properties = m_pPhysicalDevice->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    __createPlaceholderImage();
}

void CPipelineEnvironment::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboFrag", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("CombinedSampler", 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineEnvironment::__precalculateIBL(CIOImage::Ptr vSkyImage)
{
    // TODO: implement IBL 
}

void CPipelineEnvironment::__createPlaceholderImage()
{
    m_pPlaceholderImage = Function::createPlaceholderImage(m_pDevice);
}

void CPipelineEnvironment::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, m_FragUBSet[i]);
        VkImageView EnvImageView = m_pEnvironmentImage && m_pEnvironmentImage->isValid() ? *m_pEnvironmentImage : *m_pPlaceholderImage;
        WriteInfo.addWriteImageAndSampler(1, EnvImageView, m_Sampler.get());

        m_Descriptor.update(i, WriteInfo);
    }
}

void CPipelineEnvironment::__destroyResources()
{
    for (size_t i = 0; i < m_FragUBSet.size(); ++i)
    {
        m_FragUBSet[i]->destroy();
    }
    m_FragUBSet.clear();

    if (m_pEnvironmentImage) m_pEnvironmentImage->destroy();
    if (m_pPlaceholderImage) m_pPlaceholderImage->destroy();

    m_Sampler.destroy();
}