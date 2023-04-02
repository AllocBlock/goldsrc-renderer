#pragma once
#include "Pointer.h"
#include "Device.h"
#include "Log.h"
#include "Actor.h"
#include "ComponentMeshRenderer.h"
#include "VertexBuffer.h"
#include <map>

class CScene
{
public:
    _DEFINE_PTR(CScene);

    void addActor(CActor::Ptr vActor);
    size_t getActorNum() const;
    CActor::Ptr getActor(size_t vIndex) const;
    // return first found actor
    CActor::Ptr findActor(std::string vName) const;
    void clear();

    // create vertex buffer of scene, each actor's mesh data is write to the buffer as a segment
    // a actor to segment map is returned, too
    // actor without mesh will not be in the buffer nor the map

    template <typename PointData_t>
    std::pair<ptr<vk::CVertexBufferTyped<PointData_t>>, std::map<CActor::Ptr, size_t>> generateVertexBuffer(
        vk::CDevice::CPtr vDevice)
    {
        std::map<CActor::Ptr, size_t> ActorSegmentMap;
        std::vector<std::vector<PointData_t>> DataSet;
        for (auto pActor : m_ActorSet)
        {
            auto pTransform = pActor->getTransform();
            auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
            if (!pMeshRenderer) continue;

            auto pMesh = pMeshRenderer->getMesh();
            if (!pMesh) continue;

            auto MeshData = pMesh->getMeshDataV();
            const auto& Data = PointData_t::extractFromMeshData(MeshData);
            DataSet.emplace_back(Data);
            ActorSegmentMap[pActor] = DataSet.size() - 1;
        }

        auto pVertBuffer = make<vk::CVertexBufferTyped<PointData_t>>();
        if (!pVertBuffer->create(vDevice, DataSet))
        {
            if (pVertBuffer->getVertexNum() == 0)
            {
                Log::log("Warning: vertex buffer contains no vertex");
            }
            return std::make_pair(nullptr, ActorSegmentMap);
        }

        return std::make_pair(pVertBuffer, ActorSegmentMap);
    }


private:
    std::vector<CActor::Ptr> m_ActorSet;
};
