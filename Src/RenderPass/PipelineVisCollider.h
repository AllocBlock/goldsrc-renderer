#pragma once
#include "Pipeline.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Camera.h"
#include "Collider.h"

#include <map>

class CPipelineVisCollider : public IPipeline
{
public:
    void updateUniformBuffer(CCamera::CPtr vCamera);
    void startRecord(CCommandBuffer::Ptr vCommandBuffer);
    void drawCollider(CComponentCollider::CPtr vCollider);
    void endRecord();

protected:
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _initShaderResourceDescriptorV() override;
    virtual void _destroyV() override;

private:
    struct SVertexDataPos
    {
        uint32_t First;
        uint32_t Count;
    };

    void __updateDescriptorSet();
    void __initVertexBuffer();

    std::map<EBasicColliderType, SVertexDataPos> m_TypeVertexDataPosMap;
    vk::CBuffer m_VertexBuffer;
    vk::CUniformBuffer::Ptr m_pVertUniformBuffer;
    vk::CUniformBuffer::Ptr m_pFragUniformBuffer;

    CCommandBuffer::Ptr m_CurCommandBuffer = nullptr;
};