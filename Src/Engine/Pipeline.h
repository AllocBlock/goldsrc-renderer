#pragma once
#include "DrawableUI.h"
#include "Vulkan.h"
#include "ShaderResourceDescriptor.h"
#include "Device.h"
#include "PipelineDescriptor.h"
#include "CommandBuffer.h"
#include "RenderInfoDescriptor.h"
#include <filesystem>
#include <vulkan/vulkan.h> 

struct IPipeline : public IDrawableUI
{
public:
    IPipeline() = default;
    virtual ~IPipeline() = default;

    void create(cptr<vk::CDevice> vDevice, const CRenderInfoDescriptor& vRenderInfoDescriptor, VkExtent2D vExtent);
    void destroy();
    void bind(sptr<CCommandBuffer> vCommandBuffer);
    bool isValid() const { return m_Pipeline != VK_NULL_HANDLE; }

    const CShaderResourceDescriptor& getDescriptor() const { return m_ShaderResourceDescriptor; }

protected:
    /*
    * _initShaderResourceDescriptorV:
    * triggers only once
    * trigger at beginning of creation to get shader param layout
    */
    virtual void _initShaderResourceDescriptorV() = 0;

    /*
    * _renderUIV:
    * triggers only once
    * trigger in creation, after _initShaderResourceDescriptorV and before _createV
    */
    virtual CPipelineDescriptor _getPipelineDescriptionV() = 0;

    /*
    * _createV:
    * triggers only once
    * trigger when create
    */
    virtual void _createV() {};
    
    /*
    * _renderUIV:
    * triggers each frame
    * for ui drawing
    */
    virtual void _renderUIV() override {};

    /*
    * _initPushConstantV:
    * triggers multiple times
    * trigger after each bind action to give the push constant initial data
    */
    virtual void _initPushConstantV(sptr<CCommandBuffer> vCommandBuffer) {}

    /*
    * _destroyV:
    * triggers only once
    * trigger when destroy
    */
    virtual void _destroyV() {};

    VkExtent2D m_Extent = VkExtent2D{ 0, 0 };

    CShaderResourceDescriptor m_ShaderResourceDescriptor;

    cptr<vk::CDevice> m_pDevice = nullptr;
    VkPipeline m_Pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;

private:
    // no copy
    IPipeline(const IPipeline&) = delete;
    IPipeline& operator=(const IPipeline&) = delete;
};
