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

void CPipelinePBS::setSkyBoxImage(const std::array<std::shared_ptr<CIOImage>, 6>& vSkyBoxImageSet)
{
    // format 6 image into one cubemap image
    int TexWidth = vSkyBoxImageSet[0]->getWidth();
    int TexHeight = vSkyBoxImageSet[0]->getHeight();
    size_t SingleFaceImageSize = static_cast<size_t>(4) * TexWidth * TexHeight;
    size_t TotalImageSize = SingleFaceImageSize * 6;
    uint8_t* pPixelData = new uint8_t[TotalImageSize];
    memset(pPixelData, 0, TotalImageSize);
    /*
     * a cubemap image in vulkan has 6 faces(layers), and in sequence they are
     * +x, -x, +y, -y, +z, -z
     *
     * in vulkan:
     * +y
     * +z +x -z -x
     * -y
     *
     * cubemap face to outside(fold +y and -y behind)
     * in GoldSrc:
     * up
     * right front left back
     * down
     * in sequence: front back up down right left
     */

    for (size_t i = 0; i < vSkyBoxImageSet.size(); ++i)
    {
        _ASSERTE(TexWidth == vSkyBoxImageSet[i]->getWidth() && TexHeight == vSkyBoxImageSet[i]->getHeight());
        const void* pData = vSkyBoxImageSet[i]->getData();
        memcpy_s(pPixelData + i * SingleFaceImageSize, SingleFaceImageSize, pData, SingleFaceImageSize);
    }

    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = TexWidth;
    ImageInfo.extent.height = TexHeight;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 6;
    ImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    ImageInfo.flags = VkImageCreateFlagBits::VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT; // important for cubemap

    vk::SImageViewInfo ViewInfo;
    ViewInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    ViewInfo.ViewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;

    m_pSkyBoxImage = std::make_shared<vk::CImage>();
    m_pSkyBoxImage->create(m_PhysicalDevice, m_Device, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
    m_pSkyBoxImage->stageFill(pPixelData, TotalImageSize);
    delete[] pPixelData;

    // the sky image changed, so descriptor need to be updated
    // FIXME: 
    if (isReady())
        __updateDescriptorSet();
}

void CPipelinePBS::setMaterialBuffer(std::shared_ptr<vk::CBuffer> vMaterialBuffer)
{
    _ASSERTE(vMaterialBuffer);
    m_pMaterialBuffer = vMaterialBuffer;

    // FIXME: 
    if (isReady())
        __updateDescriptorSet();
}

void CPipelinePBS::setTextures(const std::vector<vk::CImage::Ptr>& vColorSet, const std::vector<vk::CImage::Ptr>& vNormalSet, const std::vector<vk::CImage::Ptr>& vSpecularSet)
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
    // FIXME: 
    if (isReady())
        __updateDescriptorSet();
}

void CPipelinePBS::__createPlaceholderImage()
{
    // placeholder image
    uint8_t Data = 0;
    auto pTinyImage = std::make_shared<CIOImage>();
    pTinyImage->setSize(1, 1);
    pTinyImage->setData(&Data);
    m_pPlaceholderImage = Function::createImageFromIOImage(m_PhysicalDevice, m_Device, pTinyImage);
}

void CPipelinePBS::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferSet[i]->get();
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SUBOVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} ,{} }));

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBufferSet[i]->get();
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SUBOFrag);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {FragBufferInfo }, {} }));

        VkDescriptorImageInfo CombinedSamplerInfo = {};
        CombinedSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        CombinedSamplerInfo.imageView = m_pSkyBoxImage->isValid() ? m_pSkyBoxImage->get() : m_pPlaceholderImage->get();
        CombinedSamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {CombinedSamplerInfo} }));

        VkDescriptorBufferInfo FragMaterialBufferInfo = {};
        FragMaterialBufferInfo.buffer = m_pMaterialBuffer ? m_pMaterialBuffer->get() : nullptr;
        FragMaterialBufferInfo.offset = 0;
        FragMaterialBufferInfo.range = m_pMaterialBuffer->getSize();
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ { FragMaterialBufferInfo }, {} }));

        VkDescriptorImageInfo SamplerInfo = {};
        SamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        SamplerInfo.imageView = VK_NULL_HANDLE;
        SamplerInfo.sampler = m_Sampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {SamplerInfo} }));

        std::vector<VkDescriptorImageInfo> TexImageColorInfoSet(CPipelinePBS::MaxTextureNum);
        for (size_t i = 0; i < CPipelinePBS::MaxTextureNum; ++i)
        {
            TexImageColorInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            TexImageColorInfoSet[i].imageView = m_TextureColorSet[i]->get();
            TexImageColorInfoSet[i].sampler = VK_NULL_HANDLE;
        }
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, TexImageColorInfoSet }));

        std::vector<VkDescriptorImageInfo> TexImageNormalInfoSet(CPipelinePBS::MaxTextureNum);
        for (size_t i = 0; i < CPipelinePBS::MaxTextureNum; ++i)
        {
            TexImageNormalInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            TexImageNormalInfoSet[i].imageView = m_TextureNormalSet[i]->get();
            TexImageNormalInfoSet[i].sampler = VK_NULL_HANDLE;
        }

        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, TexImageNormalInfoSet }));

        std::vector<VkDescriptorImageInfo> TexImageSpecularInfoSet(CPipelinePBS::MaxTextureNum);
        for (size_t i = 0; i < CPipelinePBS::MaxTextureNum; ++i)
        {
            TexImageSpecularInfoSet[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            TexImageSpecularInfoSet[i].imageView = m_TextureSpecularSet[i]->get();
            TexImageSpecularInfoSet[i].sampler = VK_NULL_HANDLE;
        }

        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, TexImageSpecularInfoSet }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
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
        m_VertUniformBufferSet[i] = std::make_shared<vk::CUniformBuffer>();
        m_VertUniformBufferSet[i]->create(m_PhysicalDevice, m_Device, VertBufferSize);
        m_FragUniformBufferSet[i] = std::make_shared<vk::CUniformBuffer>();
        m_FragUniformBufferSet[i]->create(m_PhysicalDevice, m_Device, FragBufferSize);
    }

    VkPhysicalDeviceProperties Properties = {};
    vkGetPhysicalDeviceProperties(m_PhysicalDevice, &Properties);

    VkSamplerCreateInfo SamplerInfo = {};
    SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerInfo.magFilter = VK_FILTER_LINEAR;
    SamplerInfo.minFilter = VK_FILTER_LINEAR;
    SamplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    SamplerInfo.anisotropyEnable = VK_TRUE;
    SamplerInfo.maxAnisotropy = Properties.limits.maxSamplerAnisotropy;
    SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    SamplerInfo.unnormalizedCoordinates = VK_FALSE;
    SamplerInfo.compareEnable = VK_FALSE;
    SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    SamplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    SamplerInfo.mipLodBias = 0.0f;
    SamplerInfo.minLod = 0.0f;
    SamplerInfo.maxLod = 0.0f;

    Vulkan::checkError(vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler));
    Vulkan::checkError(vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_Sampler));

    __createPlaceholderImage();
}

void CPipelinePBS::_initDescriptorV()
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("CombinedSampler", 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("UboFragMaterial", 3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Sampler", 4, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("TextureColors", 5, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelinePBS::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("TextureNormals", 6, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelinePBS::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("TextureSpeculars", 7, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelinePBS::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_Device);
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

    if (m_pSkyBoxImage) m_pSkyBoxImage->destroy();
    if (m_pPlaceholderImage) m_pPlaceholderImage->destroy();

    if (m_TextureSampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device, m_TextureSampler, nullptr);
        m_TextureSampler = VK_NULL_HANDLE;
    }

    if (m_Sampler != VK_NULL_HANDLE)
    {
        vkDestroySampler(m_Device, m_Sampler, nullptr);
        m_Sampler = VK_NULL_HANDLE;
    }

    m_TextureColorSet.clear();
    m_TextureNormalSet.clear();
    m_TextureSpecularSet.clear();
    m_pMaterialBuffer = nullptr;
}