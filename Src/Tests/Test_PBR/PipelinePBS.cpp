#include "PipelinePBS.h"
#include "MaterialPBR.h"
#include "Function.h"

struct SUBOVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Model;
};

struct SUBOFrag
{
    glm::vec4 Eye;
    SMaterialPBR Material;
    uint32_t ForceUseMat = 0u;
    uint32_t UseColorTexture = 1u;
    uint32_t UseNormalTexture = 1u;
    uint32_t UseSpecularTexture = 1u;
};

size_t CPipelinePBS::MaxTextureNum = 16;

void CPipelinePBS::setMaterialBuffer(ptr<vk::CBuffer> vMaterialBuffer)
{
    _ASSERTE(vMaterialBuffer);
    m_pMaterialBuffer = vMaterialBuffer;

    // FIXME: how to update in a better way?
    if (isReady())
        __updateDescriptorSet();
}

void CPipelinePBS::setTextures(const vk::CPointerSet<vk::CImage>& vColorSet, const vk::CPointerSet<vk::CImage>& vNormalSet, const vk::CPointerSet<vk::CImage>& vSpecularSet)
{
    _ASSERTE(m_pPlaceholderImage);
    _ASSERTE(vColorSet.size() <= CPipelinePBS::MaxTextureNum);
    _ASSERTE(vNormalSet.size() <= CPipelinePBS::MaxTextureNum);
    _ASSERTE(vSpecularSet.size() <= CPipelinePBS::MaxTextureNum);
    m_TextureColorSet.resize(CPipelinePBS::MaxTextureNum);
    m_TextureNormalSet.resize(CPipelinePBS::MaxTextureNum);
    m_TextureSpecularSet.resize(CPipelinePBS::MaxTextureNum);
    for (int i = 0; i < CPipelinePBS::MaxTextureNum; ++i)
    {
        if (i < vColorSet.size())
            m_TextureColorSet[i] = vColorSet[i];
        else
            m_TextureColorSet[i] = m_pPlaceholderImage;

        if (i < vNormalSet.size())
            m_TextureNormalSet[i] = vNormalSet[i];
        else
            m_TextureNormalSet[i] = m_pPlaceholderImage;

        if (i < vSpecularSet.size())
            m_TextureSpecularSet[i] = vSpecularSet[i];
        else
            m_TextureSpecularSet[i] = m_pPlaceholderImage;
    }

    // FIXME: how to update in a better way?
    if (isReady())
        __updateDescriptorSet();
}

void CPipelinePBS::setSkyTexture(const CIOImage::Ptr vSkyImage, const CIOImage::Ptr vSkyIrrImage)
{
    if (m_pSkyImage) m_pSkyImage->destroy();
    if (m_pSkyIrrImage) m_pSkyIrrImage->destroy();
    m_pSkyImage = Function::createImageFromIOImage(m_pDevice, vSkyImage, m_MipmapLevelNum);
    m_pSkyIrrImage = Function::createImageFromIOImage(m_pDevice, vSkyIrrImage);

    // FIXME: how to update in a better way?
    if (isReady())
        __updateDescriptorSet();
}

void CPipelinePBS::__createPlaceholderImage()
{
    m_pPlaceholderImage = Function::createPlaceholderImage(m_pDevice);
}

void CPipelinePBS::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, m_FragUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(2, m_pMaterialBuffer);
        WriteInfo.addWriteSampler(3, m_Sampler.get());
        WriteInfo.addWriteImagesAndSampler(4, m_TextureColorSet);
        WriteInfo.addWriteImagesAndSampler(5, m_TextureNormalSet);
        WriteInfo.addWriteImagesAndSampler(6, m_TextureSpecularSet);
        WriteInfo.addWriteImageAndSampler(7, m_pSkyImage, m_MipmapSampler.get());
        WriteInfo.addWriteImageAndSampler(8, m_pSkyIrrImage);
        WriteInfo.addWriteImageAndSampler(9, m_pBRDFImage);

        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelinePBS::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, const SControl& vControl)
{
    SUBOVert UBOVert = {};
    UBOVert.Model = vModel;
    UBOVert.View = vView;
    UBOVert.Proj = vProj;
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.Eye = glm::vec4(vEyePos, 0.0f);
    UBOFrag.Material = vControl.Material;
    UBOFrag.ForceUseMat = vControl.ForceUseMat ? 1u : 0u;
    UBOFrag.UseColorTexture = vControl.UseColorTexture ? 1u : 0u;
    UBOFrag.UseNormalTexture = vControl.UseNormalTexture ? 1u : 0u;
    UBOFrag.UseSpecularTexture = vControl.UseSpecularTexture ? 1u : 0u;
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelinePBS::destroy()
{
    __destroyResources();
    IPipeline::destroy();
}

void CPipelinePBS::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SPBSPointData::getBindingDescription();
    voAttributeSet = SPBSPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelinePBS::_getInputAssemblyStageInfoV()
{
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

void CPipelinePBS::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    VkDeviceSize FragBufferSize = sizeof(SUBOFrag);
    m_VertUniformBufferSet.resize(vImageNum);
    m_FragUniformBufferSet.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i] = make<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
        m_FragUniformBufferSet[i] = make<vk::CUniformBuffer>();
        m_FragUniformBufferSet[i]->create(m_pDevice, FragBufferSize);
    }

    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    SamplerInfo.maxLod = static_cast<float>(m_MipmapLevelNum);
    m_MipmapSampler.create(m_pDevice, SamplerInfo);

    __createPlaceholderImage();
    CIOImage::Ptr pBRDFIOImage = make<CIOImage>("./textures/brdf.png");
    pBRDFIOImage->read();
    m_pBRDFImage = Function::createImageFromIOImage(m_pDevice, pBRDFIOImage);
}

void CPipelinePBS::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("UboFragMaterial", 2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Sampler", 3, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("TextureColors", 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelinePBS::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("TextureNormals", 5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelinePBS::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("TextureSpeculars", 6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelinePBS::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("TextureSky", 7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("TextureSkyIrr", 8, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("TextureBRDF", 9, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

void CPipelinePBS::__destroyResources()
{
    for (size_t i = 0; i < m_VertUniformBufferSet.size(); ++i)
    {
        m_VertUniformBufferSet[i]->destroy();
        m_FragUniformBufferSet[i]->destroy();
    }
    m_VertUniformBufferSet.clear();
    m_FragUniformBufferSet.clear();

    if (m_pPlaceholderImage) m_pPlaceholderImage->destroy();

    m_Sampler.destroy();
    m_MipmapSampler.destroy();

    m_TextureColorSet.clear();
    m_TextureNormalSet.clear();
    m_TextureSpecularSet.clear();
    if (m_pSkyImage) m_pSkyImage->destroy();
    m_pSkyImage = nullptr;
    if (m_pSkyIrrImage) m_pSkyIrrImage->destroy();
    m_pSkyIrrImage = nullptr;
    if (m_pBRDFImage) m_pBRDFImage->destroy();
    m_pBRDFImage = nullptr;

    m_pMaterialBuffer = nullptr;
}