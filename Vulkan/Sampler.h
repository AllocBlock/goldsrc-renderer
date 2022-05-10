#pragma once
#include "VulkanHandle.h"
#include "Common.h"

namespace vk
{
    enum ESamplerFilterType
    {
        NEAREST,
        LINEAR,
    };

    struct CSamplerInfoGenerator
    {
        void setFilterType(VkFilter vType)
        {
            m_MagFilterType = m_MinFilterType = vType;
        }

        void setFilterType(VkFilter vMagType, VkFilter vMinType)
        {
            m_MagFilterType = vMagType;
            m_MinFilterType = vMinType;
        }

        void setAddressMode(VkSamplerAddressMode vMode)
        {
            m_AddressModeU = m_AddressModeV = m_AddressModeW = vMode;
        }

        void setMaxAnisotropy(float vMax)
        {
            _ASSERTE(vMax >= 1.0f);
            m_MaxAnisotropy = vMax;
        }

        void setMaxLod(float vMax)
        {
            _ASSERTE(vMax >= 0.0f);
            m_MaxLod = vMax;
        }

        VkSamplerCreateInfo generateCreateInfo()
        {
            VkSamplerCreateInfo SamplerInfo = {};
            SamplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            SamplerInfo.magFilter = m_MagFilterType;
            SamplerInfo.minFilter = m_MinFilterType;
            SamplerInfo.addressModeU = m_AddressModeU;
            SamplerInfo.addressModeV = m_AddressModeV;
            SamplerInfo.addressModeW = m_AddressModeW;
            SamplerInfo.anisotropyEnable = m_MaxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
            SamplerInfo.maxAnisotropy = m_MaxAnisotropy;
            SamplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            SamplerInfo.unnormalizedCoordinates = VK_FALSE;
            SamplerInfo.compareEnable = VK_FALSE;
            SamplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            SamplerInfo.mipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
            SamplerInfo.mipLodBias = 0.0f;
            SamplerInfo.minLod = 0.0f;
            SamplerInfo.maxLod = m_MaxLod;

            return SamplerInfo;
        }

        static VkSamplerCreateInfo generateCreateInfo(VkFilter vFilterType, VkSamplerAddressMode vAddressMode, float vMaxAnisotropy = 1.0f, float vMaxLod = 0.0f)
        {
            CSamplerInfoGenerator Gen;
            Gen.setFilterType(vFilterType);
            Gen.setAddressMode(vAddressMode);
            Gen.setMaxAnisotropy(vMaxAnisotropy);
            Gen.setMaxLod(vMaxLod);
            return Gen.generateCreateInfo();
        }

    private:
        VkFilter m_MagFilterType = VkFilter::VK_FILTER_LINEAR;
        VkFilter m_MinFilterType = VkFilter::VK_FILTER_LINEAR;
        VkSamplerAddressMode m_AddressModeU = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode m_AddressModeV = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode m_AddressModeW = VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
        float m_MaxAnisotropy = 1.0f;
        VkSamplerMipmapMode m_MipmapMode = VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
        float m_MaxLod = 0.0f;

    };

    class CSampler : public IVulkanHandle<VkSampler>
    {
    public:
        _DEFINE_PTR(CSampler);

        void create(VkDevice vDevice, const VkSamplerCreateInfo& vInfo);
        void destroy();

    private:
        VkDevice m_Device = VK_NULL_HANDLE;
    };
}
