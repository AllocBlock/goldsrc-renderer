#pragma once
#include "Pipeline.h"
#include "VertexAttributeDescriptor.h"
#include "Camera.h"

#include <glm/glm.hpp>
#include <map>

enum class E3DPrimitiveType
{
    SPHERE,
    CUBE,
};

class CPipelineVisualize3DPrimitive : public IPipeline
{
public:
    void add(E3DPrimitiveType vPrimitiveType, const glm::vec3& vCenter, const glm::vec3& vScale, const glm::vec3& vColor = glm::vec3(1.0, 1.0, 1.0));
    void clear();
    void updateUniformBuffer(CCamera::CPtr vCamera);
    void recordCommand(CCommandBuffer::Ptr vCommandBuffer);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _createV() override;
    virtual void _initPushConstantV(CCommandBuffer::Ptr vCommandBuffer) override;
    virtual void _destroyV() override;


private:
    struct SPrimitiveDataInfo
    {
        size_t Start;
        size_t Count;
    };

    struct SPrimitiveInfo
    {
        glm::vec3 Center;
        glm::vec3 Scale;
        glm::vec3 Color;
    };
    
    void __updateDescriptorSet();
    void __createVertexBuffer();

    std::map<E3DPrimitiveType, std::vector<SPrimitiveInfo>> m_Primitives;
    std::map<E3DPrimitiveType, SPrimitiveDataInfo> m_PrimitiveDataInfoMap;

    size_t m_VertexNum = 0;
    vk::CBuffer m_VertexBuffer;
    vk::CUniformBuffer::Ptr m_pVertUniformBuffer;
    vk::CUniformBuffer::Ptr m_pFragUniformBuffer;
};
