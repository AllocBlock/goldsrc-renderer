#pragma once
#include "PchVulkan.h"
#include "Command.h"
#include "DrawableUI.h"
#include "RenderPassPort.h"
#include "RenderInfoDescriptor.h"
#include "SceneInfo.h"

#include <string>
#include <vector>

namespace engine
{
    class IRenderPass : public IDrawableUI, public std::enable_shared_from_this<IRenderPass>
    {
    public:
        
        IRenderPass();
        virtual ~IRenderPass() = default;

        void createPortSet();
        void init(cptr<vk::CDevice> vDevice, VkExtent2D vScreenExtent);
        void update();
        std::vector<VkCommandBuffer> requestCommandBuffers();
        void destroy();

        sptr<CPortSet> getPortSet() const { return m_pPortSet; }

        sptr<SSceneInfo> getScene() const { return m_pSceneInfo; }
        void setSceneInfo(sptr<SSceneInfo> vScene)
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
        virtual sptr<CPortSet> _createPortSetV() = 0;

        /*
            * _createV:
            * triggers only once
            * AppInfo and PortSet is ready before trigger
            */
        virtual void _initV() {}

        /*
            * _updateV:
            * triggers each frame
            * update function each frame/tick
            */
        virtual void _updateV() {}

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
        virtual std::vector<VkCommandBuffer> _requestCommandBuffersV() = 0;

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
        virtual void _onSceneInfoSet(sptr<SSceneInfo> vSceneInfo) {}

        virtual std::vector<std::string> _getSecondaryCommandBufferNamesV() const { return {}; }

        void _beginCommand(sptr<CCommandBuffer> vCommandBuffer);
        void _beginRendering(sptr<CCommandBuffer> vCommandBuffer, const VkRenderingInfo& vBeginInfo);
        void _addPassBarrier(sptr<CCommandBuffer> vCommandBuffer);
        void _endRendering();
        void _endCommand();
        void _beginSecondaryCommand(sptr<CCommandBuffer> vCommandBuffer, const CRenderInfoDescriptor& vRenderInfoDescriptor);

        bool _dumpInputPortExtent(std::string vName, VkExtent2D& voExtent);

        sptr<CCommandBuffer> _getCommandBuffer();
        void _initImageLayouts(sptr<CCommandBuffer> vCommandBuffer);
        //void _beginSecondary(sptr<CCommandBuffer> vCommandBuffer);

        cptr<vk::CDevice> m_pDevice = nullptr;
        VkExtent2D m_ScreenExtent = vk::ZeroExtent;
        sptr<CPortSet> m_pPortSet = nullptr;
        sptr<SSceneInfo> m_pSceneInfo = nullptr;

        CCommand m_Command = CCommand();
        std::string m_DefaultCommandName = "Default";

    private:
        void __createCommandPoolAndBuffers();
        void __destroyCommandPoolAndBuffers();

        bool m_CommandBegun = false;
        sptr<CCommandBuffer> m_pCurrentCommandBuffer = nullptr;
    };
}
