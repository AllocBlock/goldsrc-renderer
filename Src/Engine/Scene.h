#pragma once
#include "Pointer.h"
#include "Device.h"
#include "Log.h"
#include "Actor.h"
#include "ComponentMeshRenderer.h"
#include "VertexBuffer.h"
#include "Camera.h"

#include <map>

class CScene
{
public:
    
    // actor
    void addActor(sptr<CActor> vActor);
    size_t getActorNum() const;
    sptr<CActor> getActor(size_t vIndex) const;
    // return first found actor
    sptr<CActor> findActor(std::string vName) const;
    void clear();

    // camera
    sptr<CCamera> getMainCamera() { return m_MainCamera; }

    // create vertex buffer of scene, each actor's mesh data is write to the buffer as a segment
    // a actor to segment map is returned, too
    // actor without mesh will not be in the buffer nor the map

    template <typename PointData_t>
    std::pair<sptr<vk::CVertexBufferTyped<PointData_t>>, std::map<sptr<CActor>, size_t>> generateVertexBuffer(
        cptr<vk::CDevice> vDevice)
    {
        std::map<sptr<CActor>, size_t> ActorSegmentMap;
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
            if (Data.empty()) continue;

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

    static bool isActorInSight(sptr<CActor> vActor, const SFrustum& vFrustum)
    {
        auto pTransform = vActor->getTransform();
        auto pMeshRenderer = pTransform->findComponent<CComponentMeshRenderer>();
        if (!pMeshRenderer) return false;

        auto pMesh = pMeshRenderer->getMesh();
        if (!pMesh) return false;

        auto AABB = pMesh->getAABB();
        if (!AABB.IsValid) return false;

        // AABB frustum culling
        const std::array<glm::vec4, 6>& FrustumPlanes = vFrustum.Planes;
        if (!AABB.IsValid) return false;

        std::array<glm::vec3, 8> BoundPoints = {};
        for (int i = 0; i < 8; ++i)
        {
            float X = ((i & 1) ? AABB.Min.x : AABB.Max.x);
            float Y = ((i & 2) ? AABB.Min.y : AABB.Max.y);
            float Z = ((i & 4) ? AABB.Min.z : AABB.Max.z);
            BoundPoints[i] = glm::vec3(X, Y, Z);
        }

        // for each frustum plane
        for (int i = 0; i < 6; ++i)
        {
            glm::vec3 Normal = glm::vec3(FrustumPlanes[i].x, FrustumPlanes[i].y, FrustumPlanes[i].z);
            float D = FrustumPlanes[i].w;
            // if all of the vertices in bounding is behind this plane, the object should not be drawn
            bool NoDraw = true;
            for (int k = 0; k < 8; ++k)
            {
                if (glm::dot(Normal, BoundPoints[k]) + D > 0)
                {
                    NoDraw = false;
                    break;
                }
            }
            if (NoDraw) return false;
        }
        return true;
    }

private:
    std::vector<sptr<CActor>> m_ActorSet;
    sptr<CCamera> m_MainCamera = make<CCamera>();
};
