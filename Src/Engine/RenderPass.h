#pragma once
#include "PchVulkan.h"
#include "Command.h"
#include "DrawableUI.h"
#include "RenderPassPort.h"
#include "RenderPassDescriptor.h"
#include "FrameBuffer.h"
#include "SceneInfo.h"

#include <string>
#include <vector>

namespace vk
{
    class IRenderPass : public IVulkanHandle<VkRenderPass>, public IDrawableUI, public std::enable_shared_from_this<IRenderPass>
    {
    public:
        _DEFINE_PTR(IRenderPass);

        IRenderPass();
        virtual ~IRenderPass() = default;

        void createPortSet();
        void init(CDevice::CPtr vDevice, size_t vImageNum, VkExtent2D vScreenExtent);
        void update(uint32_t vImageIndex);
        std::vector<VkCommandBuffer> requestCommandBuffers(uint32_t vImageIndex);
        void destroy();
        
        CPortSet::Ptr getPortSet() const { return m_pPortSet; }
        
        ptr<SSceneInfo> getScene() const { return m_pSceneInfo; }
        void setSceneInfo(ptr<SSceneInfo> vScene)
        {
            _ASSERTE(vScene);
            m_pSceneInfo = vScene;
            _onSceneInfoSet(vScene);
        }

    protected:

        /*
         * _createPortSetV:
         * triggers only once
         * setup PortSet
         */
        virtual CPortSet::Ptr _createPortSetV() = 0;

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
         * _onSceneInfoSet:
         * can trigger multiple times
         * trigger when scene is set
         */
        virtual void _onSceneInfoSet(ptr<SSceneInfo> vSceneInfo) {}

        void _begin(CCommandBuffer::Ptr vCommandBuffer, CFrameBuffer::CPtr vFrameBuffer, const std::vector<VkClearValue>& vClearValues, bool vHasSecondary = false);
        void _end();
        bool _dumpInputPortExtent(std::string vName, VkExtent2D& voExtent);

        CDevice::CPtr m_pDevice = nullptr;
        size_t m_ImageNum = 0;
        VkExtent2D m_ScreenExtent = vk::ZeroExtent;
        CPortSet::Ptr m_pPortSet = nullptr;
        ptr<SSceneInfo> m_pSceneInfo = nullptr;

    private:
        void __createRenderpass();
        void __destroyRenderpass();

        bool m_Begined = false;
        CCommandBuffer::Ptr m_pCurrentCommandBuffer = nullptr;
        CRenderPassDescriptor m_CurPassDesc = CRenderPassDescriptor(false);
    };
}
