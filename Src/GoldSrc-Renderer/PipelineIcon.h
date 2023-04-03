#pragma once
#include "Pipeline.h"
#include "IOImage.h"
#include "SceneInfoGoldSrc.h"
#include "Image.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Sampler.h"
#include "Camera.h"
#include "IconManager.h"

#include <glm/glm.hpp>

class CPipelineIcon : public IPipeline
{
public:
    void addIcon(EIcon vIcon, glm::vec3 vPosition, float vScale = 32.0f);
    void clear();
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void recordCommand(CCommandBuffer::Ptr vCommandBuffer, size_t vImageIndex);

protected:
    virtual void _initShaderResourceDescriptorV() override;
    virtual CPipelineDescriptor _getPipelineDescriptionV() override;
    virtual void _initPushConstantV(CCommandBuffer::Ptr vCommandBuffer) override;
    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _destroyV() override;

private:
    static const size_t MaxIconNum;

    struct SIconInfo
    {
        EIcon Icon;
        glm::vec3 Position;
        float Scale;
    };

    void __createIconResources();
    void __updateDescriptorSet();

    vk::CSampler m_Sampler;
    vk::CImage m_PlaceholderImage;
    ptr<vk::CBuffer> m_pVertexDataBuffer;
    size_t m_VertexNum = 0;
    vk::CPointerSet<vk::CUniformBuffer> m_VertUniformBufferSet;

    uint32_t m_IconNum = 0;
    vk::CPointerSet<vk::CImage> m_IconImageSet;
    std::map<EIcon, uint32_t> m_IconIndexMap;

    std::vector<SIconInfo> m_IconInfoSet;
};

