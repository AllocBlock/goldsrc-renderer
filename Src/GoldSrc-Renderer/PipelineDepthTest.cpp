#include "PipelineDepthTest.h"
#include "Sampler.h"

size_t CPipelineDepthTest::MaxTextureNum = 2048; // if need change, you should change this in frag shader as well

struct SPushConstant
{
    VkBool32 UseLightmap = VK_FALSE;
    float Opacity = 1.0;
};

struct SUBOVert
{
    alignas(16) glm::mat4 Proj;
    alignas(16) glm::mat4 View;
    alignas(16) glm::mat4 Model;
};

struct SUBOFrag
{
    alignas(16) glm::vec3 Eye;
};

void CPipelineDepthTest::updateDescriptorSet(const std::vector<VkImageView>& vTextureSet, VkImageView vLightmap)
{
    size_t DescriptorNum = m_Descriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, m_FragUniformBufferSet[i]);
        WriteInfo.addWriteSampler(2, m_Sampler.get());

        const size_t NumTexture = vTextureSet.size();

        std::vector<VkImageView> TexImageViewSet(CPipelineDepthTest::MaxTextureNum);
        for (size_t i = 0; i < CPipelineDepthTest::MaxTextureNum; ++i)
        {
            // for unused element, fill like the first one (weird method but avoid validation warning)
            if (i >= NumTexture)
            {
                if (i == 0) // no texture, use default placeholder texture
                    TexImageViewSet[i] = *m_pPlaceholderImage;
                else
                    TexImageViewSet[i] = TexImageViewSet[0];
            }
            else
                TexImageViewSet[i] = vTextureSet[i];
        }
        WriteInfo.addWriteImagesAndSampler(3, TexImageViewSet);
        VkImageView LightmapImageView = vLightmap == VK_NULL_HANDLE ? *m_pPlaceholderImage : vLightmap;
        WriteInfo.addWriteImageAndSampler(4, LightmapImageView);

        m_Descriptor.update(i, WriteInfo);
    }
}

void CPipelineDepthTest::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, glm::mat4 vView, glm::mat4 vProj, glm::vec3 vEyePos)
{
    SUBOVert UBOVert = {};
    UBOVert.Model = vModel;
    UBOVert.View = vView;
    UBOVert.Proj = vProj;
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.Eye = vEyePos;
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineDepthTest::setLightmapState(VkCommandBuffer vCommandBuffer, bool vEnable)
{
    if (m_EnableLightmap == vEnable) return;
    else
    {
        m_EnableLightmap = vEnable;
        __updatePushConstant(vCommandBuffer, vEnable, m_Opacity);
    }
}

void CPipelineDepthTest::setOpacity(VkCommandBuffer vCommandBuffer, float vOpacity)
{
    if (m_Opacity == vOpacity) return;
    else
    {
        m_Opacity = vOpacity;
        __updatePushConstant(vCommandBuffer, m_EnableLightmap, vOpacity);
    }
}

void CPipelineDepthTest::destroy()
{
    __destroyResources();
    IPipeline::destroy();
}

void CPipelineDepthTest::_getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet)
{
    voBinding = SGoldSrcPointData::getBindingDescription();
    voAttributeSet = SGoldSrcPointData::getAttributeDescriptionSet();
}

VkPipelineInputAssemblyStateCreateInfo CPipelineDepthTest::_getInputAssemblyStageInfoV()
{
    auto Info = IPipeline::getDefaultInputAssemblyStageInfo();
    Info.topology = VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    return Info;
}

VkPipelineDepthStencilStateCreateInfo CPipelineDepthTest::_getDepthStencilInfoV()
{
    VkPipelineDepthStencilStateCreateInfo DepthStencilInfo = {};
    DepthStencilInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    DepthStencilInfo.depthTestEnable = VK_TRUE;
    DepthStencilInfo.depthWriteEnable = VK_TRUE;
    DepthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    DepthStencilInfo.depthBoundsTestEnable = VK_FALSE;
    DepthStencilInfo.stencilTestEnable = VK_FALSE;

    return DepthStencilInfo;
}

std::vector<VkDynamicState> CPipelineDepthTest::_getEnabledDynamicSetV()
{
    return { VK_DYNAMIC_STATE_DEPTH_BIAS };
}

std::vector<VkPushConstantRange> CPipelineDepthTest::_getPushConstantRangeSetV()
{
    VkPushConstantRange PushConstantInfo = {};
    PushConstantInfo.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    PushConstantInfo.offset = 0;
    PushConstantInfo.size = sizeof(SPushConstant);

    return { PushConstantInfo };
}

void CPipelineDepthTest::_createResourceV(size_t vImageNum)
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

    uint8_t PixelData = 0;
    VkImageCreateInfo ImageInfo = {};
    ImageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    ImageInfo.imageType = VK_IMAGE_TYPE_2D;
    ImageInfo.extent.width = 1;
    ImageInfo.extent.height = 1;
    ImageInfo.extent.depth = 1;
    ImageInfo.mipLevels = 1;
    ImageInfo.arrayLayers = 1;
    ImageInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    ImageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    ImageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ImageInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    ImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    ImageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vk::SImageViewInfo ViewInfo;

    m_pPlaceholderImage = make<vk::CImage>();
    m_pPlaceholderImage->create(m_pDevice, ImageInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ViewInfo);
}

void CPipelineDepthTest::_initDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_Descriptor.clear();

    m_Descriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_Descriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Sampler", 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Texture", 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelineDepthTest::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);
    m_Descriptor.add("Lightmap", 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_Descriptor.createLayout(m_pDevice);
}

void CPipelineDepthTest::_initPushConstantV(VkCommandBuffer vCommandBuffer)
{
    m_EnableLightmap = false;
    m_Opacity = 1.0;
    __updatePushConstant(vCommandBuffer, m_EnableLightmap, m_Opacity);
}

void CPipelineDepthTest::__destroyResources()
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
}

void CPipelineDepthTest::__updatePushConstant(VkCommandBuffer vCommandBuffer, bool vEnableLightmap, float vOpacity)
{
    SPushConstant PushConstant;
    PushConstant.UseLightmap = vEnableLightmap;
    PushConstant.Opacity = vOpacity;
    pushConstant(vCommandBuffer, VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
}