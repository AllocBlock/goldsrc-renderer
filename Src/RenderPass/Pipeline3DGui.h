#pragma once
#include "IPipeline.h"
#include "Vulkan.h"
#include "Buffer.h"
#include "UniformBuffer.h"
#include "Camera.h"
#include "VertexAttributeDescriptor.h"

#include <map>
#include <glm/glm.hpp>

enum class EGuiObjectType
{
    LINE
};

struct SGuiObject
{
    EGuiObjectType Type = EGuiObjectType::LINE;
    std::vector<glm::vec3> Data;
};

class CPipelineLine : public IPipeline
{
public:
    void updateUniformBuffer(uint32_t vImageIndex, CCamera::CPtr vCamera);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex);
    void setObject(std::string vName, ptr<SGuiObject> vObject);
    void removeObject(std::string vName);

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/lineShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/lineShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual void _destroyV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    void __updateDescriptorSet();
    void __updateVertexBuffer();

    std::map<std::string, ptr<SGuiObject>> m_NameObjectMap;
    size_t m_VertexNum = 0;
    ptr<vk::CBuffer> m_pVertexBuffer;
    std::vector<ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
};