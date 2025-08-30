#include "PipelineSkybox.h"
#include "VertexAttributeDescriptor.h"

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace
{
    struct SPointData
    {
        glm::vec3 Pos;

        using PointData_t = SPointData;
        _DEFINE_GET_BINDING_DESCRIPTION_FUNC;

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            CVertexAttributeDescriptor Descriptor;
            Descriptor.add(_GET_ATTRIBUTE_INFO(Pos));
            return Descriptor.generate();
        }
    };

    struct SUBOVert
    {
        alignas(16) glm::mat4 Proj;
        alignas(16) glm::mat4 View;
        alignas(16) glm::vec3 EyePosition;
    };

    struct SUBOFrag
    {
        alignas(16) glm::mat4 UpCorrection;
    };
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

    m_SkyBoxImage.create(m_pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
    m_SkyBoxImage.stageFill(pPixelData, TotalImageSize);
    delete[] pPixelData;

    // the sky image changed, so descriptor need to be updated
    __updateDescriptorSet();
}

void CPipelineSkybox::updateUniformBuffer(CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Proj = vCamera->getProjMat();
    UBOVert.View = vCamera->getViewMat();
    UBOVert.EyePosition = vCamera->getPos();
    m_pVertUniformBuffer->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    glm::vec3 FixUp = glm::normalize(glm::vec3(0.0, 1.0, 0.0));
    glm::vec3 Up = glm::normalize(vCamera->getUp());

    glm::vec3 RotationAxe = glm::cross(Up, FixUp);

    if (RotationAxe.length() < Common::Acc)
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
    m_pFragUniformBuffer->update(&UBOFrag);
}

void CPipelineSkybox::recordCommand(CCommandBuffer::Ptr vCommandBuffer)
{
    if (m_VertexBuffer.isValid() && m_SkyBoxImage.isValid())
    {
        bind(vCommandBuffer);
        vCommandBuffer->bindVertexBuffer(m_VertexBuffer);
        vCommandBuffer->draw(0, static_cast<uint32_t>(m_VertexNum));
    }
}

void CPipelineSkybox::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("CombinedSampler", 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineSkybox::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("skyShader.vert");
    Descriptor.setFragShaderPath("skyShader.frag");

    Descriptor.setVertexInputInfo<SPointData>();
    Descriptor.setInputAssembly(VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, false);
    Descriptor.setEnableDepthTest(false);
    Descriptor.setEnableDepthWrite(false);

    return Descriptor;
}

void CPipelineSkybox::_createV()
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

    m_VertexBuffer.create(m_pDevice, DataSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    m_VertexBuffer.stageFill(PointData.data(), DataSize);

    // uniform buffer
    m_pVertUniformBuffer = make<vk::CUniformBuffer>();
    m_pVertUniformBuffer->create(m_pDevice, sizeof(SUBOVert));
    m_pFragUniformBuffer = make<vk::CUniformBuffer>();
    m_pFragUniformBuffer->create(m_pDevice, sizeof(SUBOFrag));

    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);
}

void CPipelineSkybox::_destroyV()
{
    m_Sampler.destroy();
    m_SkyBoxImage.destroy();
    m_VertexBuffer.destroy();
    destroyAndClear(m_pVertUniformBuffer);
    destroyAndClear(m_pFragUniformBuffer);
}

void CPipelineSkybox::__updateDescriptorSet()
{
    CDescriptorWriteInfo WriteInfo;
    WriteInfo.addWriteBuffer(0, *m_pVertUniformBuffer);
    WriteInfo.addWriteBuffer(1, *m_pFragUniformBuffer);
    WriteInfo.addWriteImageAndSampler(2, m_SkyBoxImage, m_Sampler.get());
    m_ShaderResourceDescriptor.update(WriteInfo);
}