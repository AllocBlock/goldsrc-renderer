#include "PassFullScreen.h"
#include "RenderPassDescriptor.h"

void CRenderPassFullScreen::_initV()
{
    CRenderPassSingleFrameBuffer::_initV();
    __createVertexBuffer();
}

void CRenderPassFullScreen::_destroyV()
{
    destroyAndClear(m_pVertexBuffer);
    CRenderPassSingleFrameBuffer::_destroyV();
}

void CRenderPassFullScreen::_bindVertexBuffer(CCommandBuffer::Ptr vCommandBuffer)
{
    _ASSERTE(m_pVertexBuffer->isValid());
    vCommandBuffer->bindVertexBuffer(*m_pVertexBuffer);
}

void CRenderPassFullScreen::_drawFullScreen(CCommandBuffer::Ptr vCommandBuffer)
{
    _bindVertexBuffer(vCommandBuffer);
    vCommandBuffer->draw(0, uint32_t(m_PointDataSet.size()));
}

void CRenderPassFullScreen::__createVertexBuffer()
{
     __generateScene();

    if (!m_PointDataSet.empty())
    {
        m_pVertexBuffer = make<vk::CVertexBufferTyped<SFullScreenPointData>>();
        m_pVertexBuffer->create(m_pDevice, m_PointDataSet);
    }
}

void CRenderPassFullScreen::__generateScene()
{
    m_PointDataSet =
    {
        SFullScreenPointData{glm::vec2(-1.0f, -1.0f)},
        SFullScreenPointData{glm::vec2(-1.0f, 3.0f)},
        SFullScreenPointData{glm::vec2(3.0f, -1.0f)},
    };
}
