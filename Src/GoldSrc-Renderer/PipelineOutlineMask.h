#pragma once
#include "Pipeline.h"
#include "SceneInfo.h"
#include "Camera.h"
#include "Buffer.h"
#include "UniformBuffer.h"

class CPipelineMask : public IPipeline
{
public:
    void updateUniformBuffer(cptr<CCamera> vCamera);
    void recordCommand(sptr<CCommandBuffer> vCommandBuffer);
    void setActor(sptr<CActor> vActor);
    void removeObject();

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _destroyV() override;

private:
    void __updateDescriptorSet();
    void __updateVertexBuffer(sptr<CActor> vActor);

    uint32_t m_VertexNum = 0;
    vk::CBuffer m_VertexBuffer;
    sptr<vk::CUniformBuffer> m_pVertUniformBuffer;
};
