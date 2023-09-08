#include "PipelineSimple.h"
#include "ImageUtils.h"
#include "PointData.h"

size_t CPipelineSimple::MaxTextureNum = 2048; // if need change, you should change this in frag shader as well

namespace
{
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

void CPipelineSimple::setTextures(const vk::CPointerSet<vk::CImage>& vTextureSet)
{
    m_TextureSet.clear();
    for (size_t i = 0; i < vTextureSet.size(); ++i)
    {
        m_TextureSet.emplace_back(*vTextureSet[i]);
    }
    __updateDescriptorSet();
}

void CPipelineSimple::updateUniformBuffer(uint32_t vImageIndex, glm::mat4 vModel, CCamera::CPtr vCamera)
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

void CPipelineSimple::_initShaderResourceDescriptorV()
{
    _ASSERTE(m_pDevice);
    m_ShaderResourceDescriptor.clear();

    m_ShaderResourceDescriptor.add("UboVert", 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT);
    m_ShaderResourceDescriptor.add("UboFrag", 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Sampler", 2, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT);
    m_ShaderResourceDescriptor.add("Texture", 3, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<uint32_t>(CPipelineSimple::MaxTextureNum), VK_SHADER_STAGE_FRAGMENT_BIT);

    m_ShaderResourceDescriptor.createLayout(m_pDevice);
}

CPipelineDescriptor CPipelineSimple::_getPipelineDescriptionV()
{
    CPipelineDescriptor Descriptor;

    Descriptor.setVertShaderPath("simpleShader.vert");
    Descriptor.setFragShaderPath("simpleShader.frag");

    Descriptor.setVertexInputInfo<SSimplePointData>();

    Descriptor.setEnableDepthTest(true);
    Descriptor.setEnableDepthWrite(true);

    return Descriptor;
}

void CPipelineSimple::_createV()
{
    __destroyResources();

    VkDeviceSize VertBufferSize = sizeof(SUBOVert);
    VkDeviceSize FragBufferSize = sizeof(SUBOFrag);
    m_VertUniformBufferSet.init(m_ImageNum);
    m_FragUniformBufferSet.init(m_ImageNum);

    for (size_t i = 0; i < m_ImageNum; ++i)
    {
        m_VertUniformBufferSet[i]->create(m_pDevice, VertBufferSize);
        m_FragUniformBufferSet[i]->create(m_pDevice, FragBufferSize);
    }

    const auto& Properties = m_pDevice->getPhysicalDevice()->getProperty();
    VkSamplerCreateInfo SamplerInfo = vk::CSamplerInfoGenerator::generateCreateInfo(
        VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT, Properties.limits.maxSamplerAnisotropy
    );
    m_Sampler.create(m_pDevice, SamplerInfo);

    ImageUtils::createPlaceholderImage(m_PlaceholderImage, m_pDevice);

    __updateDescriptorSet();
}

void CPipelineSimple::_destroyV()
{
    __destroyResources();
}

void CPipelineSimple::__updateDescriptorSet()
{
    size_t DescriptorNum = m_ShaderResourceDescriptor.getDescriptorSetNum();
    for (size_t i = 0; i < DescriptorNum; ++i)
    {
        CDescriptorWriteInfo WriteInfo;
        WriteInfo.addWriteBuffer(0, *m_VertUniformBufferSet[i]);
        WriteInfo.addWriteBuffer(1, *m_FragUniformBufferSet[i]);
        WriteInfo.addWriteSampler(2, m_Sampler.get());

        //const size_t NumTexture = __getActualTextureNum();
        const size_t NumTexture = m_TextureSet.size();

        std::vector<VkImageView> TexImageViewSet(CPipelineSimple::MaxTextureNum);
        for (size_t i = 0; i < CPipelineSimple::MaxTextureNum; ++i)
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

        m_ShaderResourceDescriptor.update(i, WriteInfo);
    }
}

void CPipelineSimple::__destroyResources()
{
    m_VertUniformBufferSet.destroyAndClearAll();
    m_FragUniformBufferSet.destroyAndClearAll();
    m_PlaceholderImage.destroy();
    m_Sampler.destroy();
}
