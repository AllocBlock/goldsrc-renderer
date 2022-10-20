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
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void startRecord(VkCommandBuffer vCommandBuffer, size_t vImageIndex);
    void drawCollider(ICollider::CPtr vCollider);
    void endRecord();

protected:
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createResourceV(size_t vImageNum) override;
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
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;
    vk::CPointerSet<vk::CUniformBuffer> m_FragUniformBufferSet;

    VkCommandBuffer m_CurCommandBuffer = VK_NULL_HANDLE;
};