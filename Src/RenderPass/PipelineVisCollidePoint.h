#pragma once
#include "Pipeline.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Camera.h"
#include "Collider.h"
#include "Ticker.h"

class CPipelineVisCollidePoint : public IPipeline
{
public:
    CPipelineVisCollidePoint() { m_Ticker.start(); }

    void addCollidePoint(glm::vec3 vPos, glm::vec3 vNormal, float vShowTime = 5.0f);
    void updateUniformBuffer(CCamera::CPtr vCamera);
    void record(CCommandBuffer::Ptr vCommandBuffer, glm::vec3 vEyePos);

protected:
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _initShaderResourceDescriptorV() override;
    virtual void _destroyV() override;

private:
    struct SCollidePointInfo
    {
        glm::vec3 Pos;
        glm::vec3 Normal;
        float LeftTime = 0.0f;
    };

    void __updateDescriptorSet();
    void __initVertexBuffer();

    uint32_t m_VertexNum = 0;
    vk::CBuffer m_VertexBuffer;
    vk::CUniformBuffer::Ptr m_pVertUniformBuffer;

    CTicker<float> m_Ticker;
    std::vector<SCollidePointInfo> m_CollidePointInfoSet;
};