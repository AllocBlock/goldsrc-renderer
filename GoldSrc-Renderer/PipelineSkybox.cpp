#include "PipelineSkybox.h"

#include <glm/ext/matrix_transform.hpp>

struct SSkyUniformBufferObjectVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::vec3 EyePosition;
};

struct SSkyUniformBufferObjectFrag
{
    alignas(16) glm::mat4 UpCorrection;
};

void CPipelineSkybox::destroy()
{
    if (m_Device == VK_NULL_HANDLE) return;

    if (m_TextureSampler != VK_NULL_HANDLE)
        vkDestroySampler(m_Device, m_TextureSampler, nullptr);
    m_SkyBoxImagePack.destroy(m_Device);
    m_VertexDataPack.destroy(m_Device);
    for (size_t i = 0; i < m_VertUniformBufferPacks.size(); ++i)
    {
        m_VertUniformBufferPacks[i].destroy(m_Device);
        m_FragUniformBufferPacks[i].destroy(m_Device);
    }
    m_VertUniformBufferPacks.clear();
    m_FragUniformBufferPacks.clear();

    CPipelineBase::destroy();
}

void CPipelineSkybox::setSkyBoxImage(const std::array<std::shared_ptr<CIOImage>, 6>& vSkyBoxImageSet)
{
    // format 6 image into one cubemap image
    int TexWidth = vSkyBoxImageSet[0]->getImageWidth();
    int TexHeight = vSkyBoxImageSet[0]->getImageHeight();
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
        _ASSERTE(TexWidth == vSkyBoxImageSet[i]->getImageWidth() && TexHeight == vSkyBoxImageSet[i]->getImageHeight());
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

    VkCommandBuffer CommandBuffer = Common::beginSingleTimeBuffer();
    Common::stageFillImage(m_PhysicalDevice, m_Device, pPixelData, TotalImageSize, ImageInfo, m_SkyBoxImagePack.Image, m_SkyBoxImagePack.Memory);
    Common::endSingleTimeBuffer(CommandBuffer);
    
    delete[] pPixelData;

    m_SkyBoxImagePack.ImageView = Common::createImageView(m_Device, m_SkyBoxImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE, 6);

    // the sky image changed, so descriptor need to be updated
    __updateDescriptorSet();
}

void CPipelineSkybox::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos, glm::vec3 vUp)
{
    SSkyUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = vProj;
    UBOVert.View = vView;
    UBOVert.EyePosition = vEyePos;

    void* pData;
    ck(vkMapMemory(m_Device, m_VertUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOVert), 0, &pData));
    memcpy(pData, &UBOVert, sizeof(UBOVert));
    vkUnmapMemory(m_Device, m_VertUniformBufferPacks[vImageIndex].Memory);

    SSkyUniformBufferObjectFrag UBOFrag = {};
    glm::vec3 FixUp = glm::normalize(glm::vec3(0.0, 1.0, 0.0));
    glm::vec3 Up = glm::normalize(vUp);

    glm::vec3 RotationAxe = glm::cross(Up, FixUp);

    if (RotationAxe.length() == 0)
    {
        UBOFrag.UpCorrection = glm::mat4(1.0);
        if (glm::dot(FixUp, Up) < 0)
        {
            UBOFrag.UpCorrection[0][0] = -1.0;
            UBOFrag.UpCorrection[1][1] = -1.0;
            UBOFrag.UpCorrection[2][2] = -1.0;
        }
    }
    else
    {
        float RotationRad = glm::acos(glm::dot(FixUp, Up));
        UBOFrag.UpCorrection = glm::rotate(glm::mat4(1.0), RotationRad, RotationAxe);
    }

    ck(vkMapMemory(m_Device, m_FragUniformBufferPacks[vImageIndex].Memory, 0, sizeof(UBOFrag), 0, &pData));
    memcpy(pData, &UBOFrag, sizeof(UBOFrag));
    vkUnmapMemory(m_Device, m_FragUniformBufferPacks[vImageIndex].Memory);
}

void CPipelineSkybox::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SSimplePointData::getBindingDescription();
    voAttributeSet = SSimplePointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineSkybox::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

VkPipelineDepthStencilStateCreateInfo CPipelineSkybox::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_FALSE;
    DepthStencilInfo.depthWriteEnable = VK_FALSE;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

void CPipelineSkybox::_createResourceV(size_t vImageNum)
{
    // create plane and vertex buffer
    const std::vector<glm::vec3> Vertices =
    {
        { 1.0,  1.0,  1.0}, // 0
        {-1.0,  1.0,  1.0}, // 1
        {-1.0,  1.0, -1.0}, // 2
        { 1.0,  1.0, -1.0}, // 3
        { 1.0, -1.0,  1.0}, // 4
        {-1.0, -1.0,  1.0}, // 5
        {-1.0, -1.0, -1.0}, // 6
        { 1.0, -1.0, -1.0}, // 7
    };

    const std::vector<SSimplePointData> PointData =
    {
        {Vertices[4]}, {Vertices[0]}, {Vertices[1]}, {Vertices[4]}, {Vertices[1]}, {Vertices[5]}, // +z
        {Vertices[3]}, {Vertices[7]}, {Vertices[6]}, {Vertices[3]}, {Vertices[6]}, {Vertices[2]}, // -z
        {Vertices[0]}, {Vertices[3]}, {Vertices[2]}, {Vertices[0]}, {Vertices[2]}, {Vertices[1]}, // +y
        {Vertices[5]}, {Vertices[6]}, {Vertices[7]}, {Vertices[5]}, {Vertices[7]}, {Vertices[4]}, // -y
        {Vertices[4]}, {Vertices[7]}, {Vertices[3]}, {Vertices[4]}, {Vertices[3]}, {Vertices[0]}, // +x
        {Vertices[1]}, {Vertices[2]}, {Vertices[6]}, {Vertices[1]}, {Vertices[6]}, {Vertices[5]}, // -x
    };

    VkDeviceSize DataSize = sizeof(SSimplePointData) * PointData.size();
    m_VertexNum = PointData.size();

    VkCommandBuffer CommandBuffer = Common::beginSingleTimeBuffer();
    Common::stageFillBuffer(m_PhysicalDevice, m_Device, PointData.data(), DataSize, m_VertexDataPack.Buffer, m_VertexDataPack.Memory);
    Common::endSingleTimeBuffer(CommandBuffer);

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SSkyUniformBufferObjectVert);
    VkDeviceSize FragBufferSize = sizeof(SSkyUniformBufferObjectFrag);
    m_VertUniformBufferPacks.resize(vImageNum);
    m_FragUniformBufferPacks.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        Common::createBuffer(m_PhysicalDevice, m_Device, VertBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPacks[i].Buffer, m_VertUniformBufferPacks[i].Memory);
        Common::createBuffer(m_PhysicalDevice, m_Device, FragBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBufferPacks[i].Buffer, m_FragUniformBufferPacks[i].Memory);
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

    ck(vkCreateSampler(m_Device, &SamplerInfo, nullptr, &m_TextureSampler));
}

void CPipelineSkybox::_initDescriptorV()
{
    _ASSERTE(m_Device != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("CombinedSampler", 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_Device);
}

void CPipelineSkybox::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        std::vector<SDescriptorWriteInfo> DescriptorWriteInfoSet;

        VkDescriptorBufferInfo VertBufferInfo = {};
        VertBufferInfo.buffer = m_VertUniformBufferPacks[i].Buffer;
        VertBufferInfo.offset = 0;
        VertBufferInfo.range = sizeof(SSkyUniformBufferObjectVert);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {VertBufferInfo} ,{} }));

        VkDescriptorBufferInfo FragBufferInfo = {};
        FragBufferInfo.buffer = m_FragUniformBufferPacks[i].Buffer;
        FragBufferInfo.offset = 0;
        FragBufferInfo.range = sizeof(SSkyUniformBufferObjectFrag);
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {FragBufferInfo }, {} }));

        VkDescriptorImageInfo CombinedSamplerInfo = {};
        CombinedSamplerInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        CombinedSamplerInfo.imageView = m_SkyBoxImagePack.ImageView;
        CombinedSamplerInfo.sampler = m_TextureSampler;
        DescriptorWriteInfoSet.emplace_back(SDescriptorWriteInfo({ {}, {CombinedSamplerInfo} }));

        m_Descriptor.update(i, DescriptorWriteInfoSet);
    }
}