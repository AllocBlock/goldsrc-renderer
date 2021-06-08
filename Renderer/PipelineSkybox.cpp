#include "PipelineSkybox.h"

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
    m_SkyBoxImagePack.destory(m_Device);
    m_VertexDataPack.destory(m_Device);
    for (size_t i = 0; i < m_VertUniformBufferPacks.size(); ++i)
    {
        m_VertUniformBufferPacks[i].destory(m_Device);
        m_FragUniformBufferPacks[i].destory(m_Device);
    }
    m_VertUniformBufferPacks.clear();
    m_FragUniformBufferPacks.clear();

    CPipelineBase::destory();
}

void CPipelineSkybox::createResources(size_t vImageNum)
{
    _ASSERTE(m_pScene && m_pScene->UseSkyBox);

    // format 6 image into one cubemap image
    int TexWidth = m_pScene->SkyBoxImages[0]->getImageWidth();
    int TexHeight = m_pScene->SkyBoxImages[0]->getImageHeight();
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

    for (size_t i = 0; i < m_pScene->SkyBoxImages.size(); ++i)
    {
        _ASSERTE(TexWidth == m_pScene->SkyBoxImages[i]->getImageWidth() && TexHeight == m_pScene->SkyBoxImages[i]->getImageHeight());
        const void* pData = m_pScene->SkyBoxImages[i]->getData();
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

    stageFillImage(pPixelData, TotalImageSize, ImageInfo, m_SkyBoxImagePack);
    delete[] pPixelData;

    m_SkyBoxImagePack.ImageView = Common::createImageView(m_Device, m_SkyBoxImagePack.Image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, VkImageViewType::VK_IMAGE_VIEW_TYPE_CUBE, 6);

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

    //const std::vector<SSimplePointData> PointData =
    //{
    //    {Vertices[0]}, {Vertices[3]}, {Vertices[2]}, {Vertices[0]}, {Vertices[2]}, {Vertices[1]}, // +y
    //    {Vertices[4]}, {Vertices[7]}, {Vertices[6]}, {Vertices[4]}, {Vertices[6]}, {Vertices[5]}, // -y
    //    {Vertices[4]}, {Vertices[7]}, {Vertices[3]}, {Vertices[4]}, {Vertices[3]}, {Vertices[0]}, // +x
    //    {Vertices[1]}, {Vertices[2]}, {Vertices[6]}, {Vertices[1]}, {Vertices[6]}, {Vertices[5]}, // -x
    //    {Vertices[4]}, {Vertices[0]}, {Vertices[1]}, {Vertices[4]}, {Vertices[1]}, {Vertices[5]}, // +z
    //    {Vertices[3]}, {Vertices[7]}, {Vertices[6]}, {Vertices[3]}, {Vertices[6]}, {Vertices[2]}, // -z
    //};

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

    stageFillBuffer(PointData.data(), DataSize, m_VertexDataPack);

    // uniform buffer
    VkDeviceSize VertBufferSize = sizeof(SSkyUniformBufferObjectVert);
    VkDeviceSize FragBufferSize = sizeof(SSkyUniformBufferObjectFrag);
    m_VertUniformBufferPacks.resize(vImageNum);
    m_FragUniformBufferPacks.resize(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        __createBuffer(VertBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_VertUniformBufferPacks[i].Buffer, m_VertUniformBufferPacks[i].Memory);
        __createBuffer(FragBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_FragUniformBufferPacks[i].Buffer, m_FragUniformBufferPacks[i].Memory);
    }
}

VkPipelineVertexInputStateCreateInfo CPipelineSkybox::_getVertexInputStageInfoV()
{
    const auto& Binding = SSimplePointData::getBindingDescription();
    const auto& AttributeSet = SSimplePointData::getAttributeDescriptions();

    VkPipelineVertexInputStateCreateInfo Info = CPipelineBase::getDefaultVertexInputStageInfo();
    Info.vertexBindingDescriptionCount = 1;
    Info.pVertexBindingDescriptions = &Binding;
    Info.vertexAttributeDescriptionCount = static_cast<uint32_t>(AttributeSet.size());
    Info.pVertexAttributeDescriptions = AttributeSet.data();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineSkybox::_getInputAssemblyStageInfoV()
{
    auto Info = CPipelineBase::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
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