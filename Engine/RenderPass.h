#pragma once
#include "Vulkan.h"
#include "VulkanHandle.h"
#include "Command.h"
#include "GUI.h"
#include "RenderPassPort.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace vk
{
    enum ERenderPassPos
    {
        MIDDLE = 0x00,
        BEGIN = 0x01,
        END = 0x02
    };

    enum class EImageType
    {
        COLOR,
        DEPTH
    };

    class IRenderPass : public IVulkanHandle<VkRenderPass>, public IGUI
    {
    public:
        IRenderPass() = default;

        void init(const Vulkan::SVulkanAppInfo& vAppInfo, int vRenderPassPosBitField);
        void recreate(VkFormat vImageFormat, VkExtent2D vExtent, size_t vImageNum);
        void update(uint32_t vImageIndex);
        std::vector<VkCommandBuffer> requestCommandBuffers(uint32_t vImageIndex);
        void destroy();

        void begin(VkCommandBuffer vCommandBuffer, VkFramebuffer vFrameBuffer, VkExtent2D vRenderExtent, const std::vector<VkClearValue>& vClearValues);
        void end();

        ptr<CRenderPassLink> getLink() { return m_pLink; }
        const CRenderPassPort& getPort() const { return m_Port; }

        static VkAttachmentDescription createAttachmentDescription(int vRendererPosBitField, VkFormat vImageFormat, EImageType vType);

    protected:
        virtual void _initV() {}
        virtual CRenderPassPort _getPortV() = 0;
        virtual void _recreateV() {}
        virtual void _updateV(uint32_t vImageIndex) {}
        virtual void _renderUIV() override {}
        virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) = 0;
        virtual void _destroyV() {};

        Vulkan::SVulkanAppInfo m_AppInfo;
        ptr<CRenderPassLink> m_pLink;
        CRenderPassPort m_Port;
        int m_RenderPassPosBitField = (int)ERenderPassPos::MIDDLE;

    private:
        bool m_Begined = false;
        VkCommandBuffer m_CurrentCommandBuffer = VK_NULL_HANDLE;
    };

}
