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
    void updateUniformBuffer(cptr<CCamera> vCamera);
    void startRecord(sptr<CCommandBuffer> vCommandBuffer);
    void drawCollider(cptr<CComponentCollider> vCollider);
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
    sptr<vk::CUniformBuffer> m_pVertUniformBuffer;
    sptr<vk::CUniformBuffer> m_pFragUniformBuffer;

    sptr<CCommandBuffer> m_CurCommandBuffer = nullptr;
};