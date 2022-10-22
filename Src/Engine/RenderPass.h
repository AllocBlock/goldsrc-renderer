#pragma once
#include "PchVulkan.h"
#include "Command.h"
#include "DrawableUI.h"
#include "RenderPassPort.h"
#include "RenderPassDescriptor.h"
#include "AppInfo.h"

#include <string>
#include <vector>

namespace vk
{
    template <typename T>
    struct SPassUpdateAttribute
    {
        T Value;
        bool IsUpdated = false;

        static SPassUpdateAttribute create(const T& vOld, const T& vNew)
        {
            SPassUpdateAttribute Attr;
            Attr.Value = vNew;
            Attr.IsUpdated = (vOld != vNew);
            return Attr;
        }

        static SPassUpdateAttribute create(const T& vNew, bool vUpdated)
        {
            return SPassUpdateAttribute{vNew, vUpdated};
        }

        void operator = (const T& vValue)
        {
            Value = vValue;
            IsUpdated = false;
        }
    };

    // TIPS: pipelines require update when image extent changed, as viewport has to update
    // TIPS: pipelines require update when renderpass changed, as it depends on renderpass
    // TIPS: some textures with same extent as screen require update
    // TIPS: command require rerecord when command manager changed
    struct SPassUpdateState
    {
        SPassUpdateAttribute<VkFormat> ImageFormat;
        SPassUpdateAttribute<VkExtent2D> ImageExtent;
        SPassUpdateAttribute<size_t> ImageNum;
        bool RenderpassUpdated = false;
        bool CommandUpdated = false;
    };

    class IRenderPass : public IVulkanHandle<VkRenderPass>, public IDrawableUI
    {
    public:
        IRenderPass();

        void init(const vk::SAppInfo& vAppInfo);
        void update(uint32_t vImageIndex);
        std::vector<VkCommandBuffer> requestCommandBuffers(uint32_t vImageIndex);
        void destroy();

        void updateImageInfo(VkFormat vImageFormat, VkExtent2D vImageExtent, size_t vImageNum);

        void begin(VkCommandBuffer vCommandBuffer, VkFramebuffer vFrameBuffer, VkExtent2D vRenderExtent, const std::vector<VkClearValue>& vClearValues);
        void end();

        CPortSet::Ptr getPortSet() const { return m_pPortSet; }

    protected:
        /*
         * _getPortDescV:
         * triggers only once
         * setup port info for PortSet
         */
        virtual SPortDescriptor _getPortDescV() = 0;

        /*
         * _createV: 
         * triggers only once
         * AppInfo and PortSet is ready before trigger
         */
        virtual void _initV() {}
        
        /*
         * _getRenderPassDescV:
         * can trigger multiple times
         * return renderpass attachment info, require every time then renderpass need recreate
         */
        virtual CRenderPassDescriptor _getRenderPassDescV() = 0;

        /*
         * _updateV:
         * triggers each frame
         * update function each frame/tick
         */
        virtual void _updateV(uint32_t vImageIndex) {}
        
        /*
         * _renderUIV:
         * triggers each frame
         * drawCollider ui
         */
        virtual void _renderUIV() override {}
        
        /*
         * _requestCommandBuffersV:
         * trigger each frame
         * get command buffer each frame
         */
        virtual std::vector<VkCommandBuffer> _requestCommandBuffersV(uint32_t vImageIndex) = 0;

        /*
         * _destroyV:
         * trigger only once
         * destory everything
         */
        virtual void _destroyV() {}

        /*
         * _onUpdateV:
         * can trigger multiple times
         * trigger when something updated
         */
        virtual void _onUpdateV(const SPassUpdateState& vUpdateState) {}

        vk::SAppInfo m_AppInfo;
        CPortSet::Ptr m_pPortSet;

        CCommand m_Command = CCommand();
        std::string m_DefaultCommandName = "Default";

    private:
        void __createCommandPoolAndBuffers();
        void __destroyCommandPoolAndBuffers();
        bool __createRenderpass();
        void __destroyRenderpass();
        void __beginCommand(VkCommandBuffer vCommandBuffer);
        void __endCommand(VkCommandBuffer vCommandBuffer);

        void __triggerImageUpdate(VkFormat vOldImageFormat, VkExtent2D vOldImageExtent, size_t vOldImageNum)
        {
            SPassUpdateState State;
            State.ImageFormat = SPassUpdateAttribute<VkFormat>::create(vOldImageFormat, m_AppInfo.ImageFormat);
            State.ImageExtent = SPassUpdateAttribute<VkExtent2D>::create(vOldImageExtent, m_AppInfo.Extent);
            State.ImageNum = SPassUpdateAttribute<size_t>::create(vOldImageNum, m_AppInfo.ImageNum);
            State.RenderpassUpdated = false;
            State.CommandUpdated = false;

            _onUpdateV(State);
        }

        void __triggerRenderpassUpdate()
        {
            SPassUpdateState State;
            State.ImageFormat = m_AppInfo.ImageFormat;
            State.ImageExtent = m_AppInfo.Extent;
            State.ImageNum = m_AppInfo.ImageNum;
            State.RenderpassUpdated = true;
            State.CommandUpdated = false;

            _onUpdateV(State);
        }

        void __triggerCommandUpdate()
        {
            SPassUpdateState State;
            State.ImageFormat = m_AppInfo.ImageFormat;
            State.ImageExtent = m_AppInfo.Extent;
            State.ImageNum = m_AppInfo.ImageNum;
            State.RenderpassUpdated = false;
            State.CommandUpdated = true;

            _onUpdateV(State);
        }

        bool m_Begined = false;
        VkCommandBuffer m_CurrentCommandBuffer = VK_NULL_HANDLE;
        CRenderPassDescriptor m_CurPassDesc;
    };

}