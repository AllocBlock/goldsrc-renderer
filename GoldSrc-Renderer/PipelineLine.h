#pragma once
#include "Pipeline.h"
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
    void destroy();
    void updateUniformBuffer(uint32_t vImageIndex, ptr<CCamera> vCamera);
    void recordCommand(VkCommandBuffer vCommandBuffer, size_t vImageIndex);
    void setObject(std::string vName, ptr<SGuiObject> vObject);
    void removeObject(std::string vName);

protected:
    virtual std::filesystem::path _getVertShaderPathV() override { return "shaders/lineShaderVert.spv"; }
    virtual std::filesystem::path _getFragShaderPathV() override { return "shaders/lineShaderFrag.spv"; }

    virtual void _createResourceV(size_t vImageNum) override;
    virtual void _initDescriptorV() override;
    virtual VkPipelineDepthStencilStateCreateInfo _getDepthStencilInfoV() override;
    virtual void _getVertexInputInfoV(VkVertexInputBindingDescription& voBinding, std::vector<VkVertexInputAttributeDescription>& voAttributeSet) override;
    virtual VkPipelineInputAssemblyStateCreateInfo _getInputAssemblyStageInfoV() override;

private:
    struct SPointData
    {
        glm::vec3 Pos;

        static VkVertexInputBindingDescription getBindingDescription()
        {
            VkVertexInputBindingDescription BindingDescription = {};
            BindingDescription.binding = 0;
            BindingDescription.stride = sizeof(SPointData);
            BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return BindingDescription;
        }

        static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptionSet()
        {
            VertexAttributeDescriptor Descriptor;
            Descriptor.add(VK_FORMAT_R32G32B32_SFLOAT, offsetof(SPointData, Pos));
            return Descriptor.generate();
        }
    };

    void __updateDescriptorSet();
    void __updateVertexBuffer();

    std::map<std::string, ptr<SGuiObject>> m_NameObjectMap;
    size_t m_VertexNum = 0;
    ptr<vk::CBuffer> m_pVertexBuffer;
    std::vector<ptr<vk::CUniformBuffer>> m_VertUniformBufferSet;
};