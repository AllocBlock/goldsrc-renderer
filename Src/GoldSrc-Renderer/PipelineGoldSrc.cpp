#include "PipelineNormal.h"
#include "Sampler.h"
#include "Function.h"
#include "PointData.h"

size_t CPipelineGoldSrc::MaxTextureNum = 2048; // if need change, you should change this in frag shader as well

namespace
{
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
}

void CPipelineGoldSrc::__updateDescriptorSet()
{
    // FIXME: duplicate update exists
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, *m_FragUniformBufferSet[i]);
        WriteInfo.addWriteSampler(2, m_Sampler.get());

        const size_t NumTexture = m_TextureSet.size();

        std::vector<VkImageView> TexImageViewSet(CPipelineGoldSrc::MaxTextureNum);
        for (size_t i = 0; i < CPipelineGoldSrc::MaxTextureNum; ++i)
        {
            // for unused element, fill like the first one (weird method but avoid validation warning)
            if (i >= NumTexture)
            {
                if (i == 0) // no texture, use default placeholder texture
                    TexImageViewSet[i] = m_PlaceholderImage;
                else
                    TexImageViewSet[i] = TexImageViewSet[0];
            }
            else
                TexImageViewSet[i] = m_TextureSet[i];
        }
        WriteInfo.addWriteImagesAndSampler(3, TexImageViewSet);
        VkImageView LightmapImageView = m_LightmapTexture == VK_NULL_HANDLE ? m_PlaceholderImage : m_LightmapTexture;
        WriteInfo.addWriteImageAndSampler(4, LightmapImageView);

        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineGoldSrc::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, CCamera::CPtr vCamera)
{
    SUBOVert UBOVert = {};
    UBOVert.Model = vModel;
    UBOVert.View = vCamera->getViewMat();
    UBOVert.Proj = vCamera->getProjMat();
    m_VertUniformBufferSet[vImageIndex]->update(&UBOVert);

    SUBOFrag UBOFrag = {};
    UBOFrag.Eye = vCamera->getPos();
    m_FragUniformBufferSet[vImageIndex]->update(&UBOFrag);
}

void CPipelineGoldSrc::setLightmapState(CCommandBuffer::Ptr vCommandBuffer, bool vEnable)
{
    if (m_EnableLightmap == vEnable) return;
    else
    {
        m_EnableLightmap = vEnable;
        __updatePushConstant(vCommandBuffer, vEnable, m_Opacity);
    }
}

void CPipelineGoldSrc::setOpacity(CCommandBuffer::Ptr vCommandBuffer, float vOpacity)
{
    if (m_Opacity == vOpacity) return;
    else
    {
        m_Opacity = vOpacity;
        __updatePushConstant(vCommandBuffer, m_EnableLightmap, vOpacity);
    }
}

void CPipelineGoldSrc::setTextures(const vk::CPointerSet<vk::CImage>& vTextureSet)
{
    m_TextureSet.clear();
    for (size_t i = 0; i < vTextureSet.size(); ++i)
    {
        m_TextureSet.emplace_back(*vTextureSet[i]);
    }
    __updateDescriptorSet();
}

void CPipelineGoldSrc::setLightmap(VkImageView vLightmap)
{
    m_LightmapTexture = vLightmap;
    __updateDescriptorSet();
}

void CPipelineGoldSrc::clearResources()
{
    m_TextureSet.clear();
    m_LightmapTexture = VK_NULL_HANDLE;
    __updateDescriptorSet();
}

void CPipelineGoldSrc::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice != VK_NULL_HANDLE);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Sampler", 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Texture", 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelineGoldSrc::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Lightmap", 4, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineGoldSrc::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertexInputInfo<SGoldSrcPointData>();
    Descriptor.addPushConstant<SPushConstant>(VK_SHADER_STAGE_FRAGMENT_BIT);
    Descriptor.setDynamicStateSet({ VK_DYNAMIC_STATE_DEPTH_BIAS });

    Descriptor.setEnableDepthTest(true);
    Descriptor.setEnableDepthWrite(true);

    _dumpExtraPipelineDescriptionV(Descriptor);

    return Descriptor;
}

void CPipelineGoldSrc::_createResourceV(size_t vImageNum)
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    VkDeviceSize FragBufferSize = sizeof(SUBOFrag);
    m_VertUniformBufferSet.init(vImageNum);
    m_FragUniformBufferSet.init(vImageNum);

    for (size_t i = 0; i < vImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
        m_FragUniformBufferSet[i]->create(m_pDevice, FragBufferSize);
    }

    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    Function::createPlaceholderImage(m_PlaceholderImage, m_pDevice);
}

void CPipelineGoldSrc::_initPushConstantV(CCommandBuffer::Ptr vCommandBuffer)
{
    m_EnableLightmap = false;
    m_Opacity = 1.0;
    __updatePushConstant(vCommandBuffer, m_EnableLightmap, m_Opacity);
}

void CPipelineGoldSrc::_destroyV()
{
    __destroyResources();
}

void CPipelineGoldSrc::__destroyResources()
{
    m_VertUniformBufferSet.destroyAndClearAll();
    m_FragUniformBufferSet.destroyAndClearAll();

    m_PlaceholderImage.destroy();
    m_Sampler.destroy();
}

void CPipelineGoldSrc::__updatePushConstant(CCommandBuffer::Ptr vCommandBuffer, bool vEnableLightmap, float vOpacity)
{
    SPushConstant PushConstant;
    PushConstant.UseLightmap = vEnableLightmap;
    PushConstant.Opacity = vOpacity;
    vCommandBuffer->pushConstant(VK_SHADER_STAGE_FRAGMENT_BIT, PushConstant);
}
