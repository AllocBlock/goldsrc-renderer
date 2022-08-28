#pragma once
#include "PchVulkan.h"
#include "Command.h"
#include "IGUI.h"
#include "RenderPassPort.h"
#include "AppInfo.h"

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

    class IRenderPass : public IVulkanHandle<VkRenderPass>, public IGUI
    {
    public:
        IRenderPass() = default;

        void init(const vk::SAppInfo& vAppInfo, int vRenderPassPosBitField);
        void recreate(VkFormat vImageFormat, VkExtent2D vExtent, size_t vImageNum);
        void update(uint32_t vImageIndex);
        std::vector<VkCommandBuffer> requestCommandBuffers(uint32_t vImageIndex);
        void destroy();

        void begin(VkCommandBuffer vCommandBuffer, VkFramebuffer vFrameBuffer, VkExtent2D vRenderExtent, const std::vector<VkClearValue>& vClearValues);
        void end();

        CPortSet::Ptr getPortSet() const { return m_pPortSet; }

    protected:
        virtual void _initV() {}
        virtual SPortDescriptor _getPortDescV() = 0;
        virtual void _recreateV() {}
        virtual void _updateV(uint32_t vImageIndex) {}
        virtual void _renderUIV() override {}
        virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) = 0;
        virtual void _destroyV() {};

        vk::SAppInfo m_AppInfo;
        CPortSet::Ptr m_pPortSet;
        int m_RenderPassPosBitField = (int)ERenderPassPos::MIDDLE;

    private:
        void __beginCommand(VkCommandBuffer vCommandBuffer);
        void __endCommand(VkCommandBuffer vCommandBuffer);

        bool m_Begined = false;
        VkCommandBuffer m_CurrentCommandBuffer = VK_NULL_HANDLE;
    };

}