#pragma once
#include "Pointer.h"
#include "Device.h"
#include "Log.h"
#include "Actor.h"

enum class EAttributeType
{
    POSITION,
    NORMAL
};

class CTempScene
{
public:
    _DEFINE_PTR(CTempScene);

    void addActor(CActor::Ptr vActor)
    {
        m_ActorSet.emplace_back(vActor);
    }

    size_t getActorNum() { return m_ActorSet.size(); }
    CActor::Ptr getActor(size_t vIndex)
    {
        _ASSERTE(vIndex < m_ActorSet.size());
        return m_ActorSet[vIndex];
    }

    void clear()
    {
        m_ActorSet.clear();
    }

    template <typename PointData_t>
    vk::CBuffer::Ptr generateVertexBuffer(vk::CDevice::CPtr vDevice, size_t& voVertexNum)
    {
        size_t NumVertex = 0;
        std::vector<std::vector<PointData_t>> DataSet;
        for (auto pActor : m_ActorSet)
        {
            auto pMesh = pActor->getMesh();
            auto MeshData = pMesh->getMeshData();
            const auto& Data = PointData_t::extractFromMeshData(MeshData);
            DataSet.emplace_back(Data);
            NumVertex += Data.size();
        }

        if (NumVertex == 0)
        {
            Common::Log::log(u8"没有顶点数据，跳过顶点缓存创建");
            return nullptr;
        }

        VkDeviceSize BufferSize = sizeof(CPipelineShade::SPointData) * NumVertex;
        uint8_t* pData = new uint8_t[BufferSize];
        size_t Offset = 0;
        for (const auto& Data : DataSet)
        {
            size_t SubBufferSize = sizeof(CPipelineShade::SPointData) * Data.size();
            memcpy(reinterpret_cast<char*>(pData) + Offset, Data.data(), SubBufferSize);
            Offset += SubBufferSize;
        }

        auto pVertBuffer = make<vk::CBuffer>();
        pVertBuffer->create(vDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        pVertBuffer->stageFill(pData, BufferSize);
        delete[] pData;

        voVertexNum = NumVertex;
        return pVertBuffer;
    }

private:
    std::vector<CActor::Ptr> m_ActorSet;
};