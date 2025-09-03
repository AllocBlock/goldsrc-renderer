#pragma once
#include "RenderPass.h"
#include "Buffer.h"
#include "Scene.h"

template <typename SPointData_t>
class CRenderPassTempSceneBase : public engine::IRenderPass
{
public:
    void setScene(sptr<CScene> vScene)
    {
        m_pVertBuffer = vScene->generateVertexBuffer<SPointData_t>(m_pDevice, m_ActorDataPositionSet, m_VertexNum);
    }

protected:
    virtual void _destroyV() override
    {
        destroyAndClear(m_pVertBuffer);

        engine::IRenderPass::_destroyV();
    }
    
    sptr<vk::CBuffer> m_pVertBuffer;
    std::vector<SActorDataInfo> m_ActorDataPositionSet;
    size_t m_VertexNum = 0;
};
