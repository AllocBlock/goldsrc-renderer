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
    if (m_pDevice == VK_NULL_HANDLE) return;

    m_Sampler.destroy();
    if (m_pSkyBoxImage) m_pSkyBoxImage->destroy();
    if (m_pVertexBuffer) m_pVertexBuffer->destroy();
    for (size_t i = 0; i < m_VertUniformBufferSet.size(); ++i)
    {
        m_VertUniformBufferSet[i]->destroy();
        m_FragUniformBufferSet[i]->destroy();
    }
    m_VertUniformBufferSet.clear();
    m_FragUniformBufferSet.clear();

    IPipeline::destroy();
}

void CPipelineSkybox::setSkyBoxImage(const std::array<ptr<CIOImage>, 6>& vSkyBoxImageSet)
{
    // format 6 image into one cubemap image
    size_t TexWidth = vSkyBoxImageSet[0]->getWidth();
    size_t TexHeight = vSkyBoxImageSet[0]->getHeight();
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
    ImageInfo.extent.width = static_cast<uint32_t>(TexWidth);
    ImageInfo.extent.height = static_cast<uint32_t>(TexHeight);
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
    ViewInfo.ViewType = VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE;

    m_pSkyBoxImage = make<vk::CImage>();
    m_pSkyBoxImage->create(m_pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
    m_pSkyBoxImage->stageFill(pPixelData, TotalImageSize);
    delete[] pPixelData;

    // the sky image changed, so descriptor need to be updated
    __updateDescriptorSet();
}

void CPipelineSkybox::updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera)
{
    SSkyUniformBufferObjectVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.EyePosition = vCamera->getPos();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SSkyUniformBufferObjectFrag UBOFrag = {};
    glm::vec3 FixUp = glm::normalize(glm::vec3(0.0, 1.0, 0.0));
    glm::vec3 Up = glm::normalize(vCamera->getUp());

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
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineSkybox::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SPointData::getBindingDescription();
    voAttributeSet = SPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineSkybox::_getInputAssemblyStageInfoV()
{
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
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

    const std::vector<SPointData> PointData =
    {
        {Vertices[4]}, {Vertices[0]}, {Vertices[1]}, {Vertices[4]}, {Vertices[1]}, {Vertices[5]}, // +z
        {Vertices[3]}, {Vertices[7]}, {Vertices[6]}, {Vertices[3]}, {Vertices[6]}, {Vertices[2]}, // -z
        {Vertices[0]}, {Vertices[3]}, {Vertices[2]}, {Vertices[0]}, {Vertices[2]}, {Vertices[1]}, // +y
        {Vertices[5]}, {Vertices[6]}, {Vertices[7]}, {Vertices[5]}, {Vertices[7]}, {Vertices[4]}, // -y
        {Vertices[4]}, {Vertices[7]}, {Vertices[3]}, {Vertices[4]}, {Vertices[3]}, {Vertices[0]}, // +x
        {Vertices[1]}, {Vertices[2]}, {Vertices[6]}, {Vertices[1]}, {Vertices[6]}, {Vertices[5]}, // -x
    };

    VkDeviceSize DataSize = sizeof(SPointData) * PointData.size();
    m_VertexNum = PointData.size();

    m_pVertexBuffer = make<vk::CBuffer>();
    m_pVertexBuffer->create(m_pDevice, DataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_pVertexBuffer->stageFill(PointData.data(), DataSize);

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SSkyUniformBufferObjectVert);
    VkDeviceSize FragBufferSize = sizeof(SSkyUniformBufferObjectFrag);
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
}

void CPipelineSkybox::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("CombinedSampler", 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineSkybox::__updateDescriptorSet()
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, m_FragUniformBufferSet[i]);
        WriteInfo.addWriteImageAndSampler(2, m_pSkyBoxImage, m_Sampler.get());
        m_Descriptor.update(i, WriteInfo);
    }
}