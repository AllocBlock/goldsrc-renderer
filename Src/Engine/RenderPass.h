#pragma once
#include "PchVulkan.h"
#include "Command.h"
#include "DrawableUI.h"
#include "RenderPassPort.h"
#include "RenderPassDescriptor.h"
#include "AppInfo.h"
#include "FrameBuffer.h"

#include <string>
#include <vector>

namespace vk
{
    template <typename T>
    struct SPassUpdateAttribute
    {
        T Value = T();
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

    /*
     * TIPS:
     * 1. all image index related function react to image num change
     * 2. input image may change, which lead to update
     * 3. pipelines require update when renderpass changed, as it depends on renderpass
     *    some textures with same extent as screen require update
     * 4. command require rerecord when command manager changed
     */
    struct SPassUpdateState
    {
        SPassUpdateState()
        {
            ImageNum = 0;
            ScreenExtent = { 0, 0 };
            InputImageUpdated = false;
            RenderpassUpdated = false;
        }

        SPassUpdateState(uint32_t vImageNum, VkExtent2D vExtent)
        {
            ImageNum = vImageNum;
            ScreenExtent = vExtent;
            InputImageUpdated = false;
            RenderpassUpdated = false;
        }

        SPassUpdateAttribute<uint32_t> ImageNum;
        SPassUpdateAttribute<VkExtent2D> ScreenExtent;
        bool InputImageUpdated = false;
        bool RenderpassUpdated = false;
    };

    class IRenderPass : public IVulkanHandle<VkRenderPass>, public IDrawableUI
    {
    public:
        IRenderPass();
        virtual ~IRenderPass() = default;

        void init(CDevice::CPtr vDevice, CAppInfo::Ptr vAppInfo);
        void update(uint32_t vImageIndex);
        std::vector<VkCommandBuffer> requestCommandBuffers(uint32_t vImageIndex);
        void destroy();

        void begin(VkCommandBuffer vCommandBuffer, CFrameBuffer::CPtr vFrameBuffer, const std::vector<VkClearValue>& vClearValues);
        void end();
        
        CPortSet::Ptr getPortSet() const { return m_pPortSet; }

        static const std::vector<VkClearValue>& DefaultClearValueColor;
        static const std::vector<VkClearValue>& DefaultClearValueColorDepth;

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

        bool _dumpInputPortExtent(std::string vName, VkExtent2D& voExtent);

        CDevice::CPtr m_pDevice = nullptr;
        CAppInfo::Ptr m_pAppInfo = nullptr;
        CPortSet::Ptr m_pPortSet = nullptr;

        CCommand m_Command = CCommand();
        std::string m_DefaultCommandName = "Default";

    private:
        void __createCommandPoolAndBuffers(uint32_t vImageNum);
        void __destroyCommandPoolAndBuffers();
        void __createRenderpass();
        void __destroyRenderpass();
        void __beginCommand(VkCommandBuffer vCommandBuffer);
        void __endCommand(VkCommandBuffer vCommandBuffer);
        
        void __updateImageNum(uint32_t vImageNum);
        void __hookEvents();
        void __unhookEvents();

        void __triggerImageNumUpdate(uint32_t vImageNum);
        void __triggerScreenExtentUpdate(VkExtent2D vExtent);
        void __triggerInputImageUpdate();
        void __triggerRenderpassUpdate();

        bool m_Begined = false;
        VkCommandBuffer m_CurrentCommandBuffer = VK_NULL_HANDLE;
        CRenderPassDescriptor m_CurPassDesc = CRenderPassDescriptor(false);

        HookId_t m_ImageNumUpdateHookId = 0;
        HookId_t m_ScreenExtentUpdateHookId = 0;
        HookId_t m_InputImageUpdateHookId = 0;
        HookId_t m_LinkUpdateHookId = 0;
    };
}
