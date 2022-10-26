#pragma once
#include "Pointer.h"
#include "Device.h"
#include "Log.h"
#include "Actor.h"
#include "Buffer.h"

struct SActorDataInfo
{
    uint32_t First;
    uint32_t Num;
};

template <typename MeshData_t = CMeshDataGeneral>
class CTempScene
{
public:
    _DEFINE_PTR(CTempScene);

    void addActor(ptr<CActor<MeshData_t>> vActor)
    {
        m_ActorSet.emplace_back(vActor);
    }

    size_t getActorNum() { return m_ActorSet.size(); }
    ptr<CActor<MeshData_t>> getActor(size_t vIndex)
    {
        _ASSERTE(vIndex < m_ActorSet.size());
        return m_ActorSet[vIndex];
    }

    // return first found actor
    ptr<CActor<MeshData_t>> findActor(std::string vName)
    {
        for (auto pActor : m_ActorSet)
            if (pActor->getName() == vName)
                return pActor;
        return nullptr;
    }

    void clear()
    {
        m_ActorSet.clear();
    }

    template <typename PointData_t>
    vk::CBuffer::Ptr generateVertexBuffer(vk::CDevice::CPtr vDevice, std::vector<SActorDataInfo>& voPositionSet, size_t& voVertexNum)
    {
        uint32_t VertexNum = 0;
        std::vector<std::vector<PointData_t>> DataSet;
        std::vector<SActorDataInfo> PositionSet;
        for (auto pActor : m_ActorSet)
        {
            auto pMesh = pActor->getMesh();
            auto MeshData = pMesh->getMeshData();
            const auto& Data = PointData_t::extractFromMeshData(MeshData);
            DataSet.emplace_back(Data);

            uint32_t CurVertexNum = static_cast<uint32_t>(Data.size());
            PositionSet.push_back({ VertexNum, CurVertexNum });
            VertexNum += CurVertexNum;
        }

        if (VertexNum == 0)
        {
            Log::log("没有顶点数据，跳过顶点缓存创建");
            return nullptr;
        }

        VkDeviceSize BufferSize = sizeof(PointData_t) * VertexNum;
        uint8_t* pData = new uint8_t[BufferSize];
        size_t Offset = 0;
        for (const auto& Data : DataSet)
        {
            size_t SubBufferSize = sizeof(PointData_t) * Data.size();
            memcpy(reinterpret_cast<char*>(pData) + Offset, Data.data(), SubBufferSize);
            Offset += SubBufferSize;
        }

        auto pVertBuffer = make<vk::CBuffer>();
        pVertBuffer->create(vDevice, BufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        pVertBuffer->stageFill(pData, BufferSize);
        delete[] pData;

        voPositionSet = PositionSet;
        voVertexNum = VertexNum;
        return pVertBuffer;
    }

private:
    std::vector<ptr<CActor<MeshData_t>>> m_ActorSet;
};
