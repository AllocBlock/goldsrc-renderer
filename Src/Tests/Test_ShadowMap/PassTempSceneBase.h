#pragma once
#include "RenderPass.h"
#include "FrameBuffer.h"
#include "Camera.h"
#include "Buffer.h"
#include "Scene.h"

template <typename SPointData_t>
class CRenderPassTempSceneBase : public vk::IRenderPass
{
public:
    void setScene(CScene::Ptr vScene)
    {
        m_pVertBuffer = vScene->generateVertexBuffer<SPointData_t>(m_pDevice, m_ActorDataPositionSet, m_VertexNum);
    }

protected:
    virtual void _destroyV() override
    {
        destroyAndClear(m_pVertBuffer);

        vk::IRenderPass::_destroyV();
    }
    
    ptr<vk::CBuffer> m_pVertBuffer;
    std::vector<SActorDataInfo> m_ActorDataPositionSet;
    size_t m_VertexNum = 0;
};
