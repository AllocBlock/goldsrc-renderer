#pragma once
#include "Pointer.h"
#include "Device.h"
#include "Log.h"
#include "Actor.h"
#include "VertexBuffer.h"

class CScene
{
public:
    _DEFINE_PTR(CScene);

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

    // return first found actor
    CActor::Ptr findActor(std::string vName)
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
    ptr<vk::CVertexBufferTyped<PointData_t>> generateVertexBuffer(vk::CDevice::CPtr vDevice)
    {
        std::vector<std::vector<PointData_t>> DataSet;
        for (auto pActor : m_ActorSet)
        {
            auto pMesh = pActor->getMesh();
            auto MeshData = pMesh->getMeshDataV();
            const auto& Data = PointData_t::extractFromMeshData(MeshData);
            DataSet.emplace_back(Data);
        }
        
        auto pVertBuffer = make<vk::CVertexBufferTyped<PointData_t>>();
        if (!pVertBuffer->create(vDevice, DataSet))
        {
            if (pVertBuffer->getVertexNum() == 0)
            {
                Log::log("Warning: vertex buffer contains no vertex");
            }
            return nullptr;
        }

        return pVertBuffer;
    }

private:
    std::vector<CActor::Ptr> m_ActorSet;
};
