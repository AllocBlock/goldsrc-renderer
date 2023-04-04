#pragma once
#include "RenderPassSingle.h"
#include "Camera.h"
#include "VertexBuffer.h"
#include "BoundingBox.h"
#include "ComponentMeshRenderer.h"
#include "SceneInfoGoldSrc.h"

class CRenderPassScene : public CRenderPassSingle
{
public:
    _DEFINE_PTR(CRenderPassScene);

    CCamera::Ptr getCamera() { return m_pCamera; }
    void setCamera(CCamera::Ptr vCamera) { m_pCamera = vCamera; }

    ptr<SSceneInfoGoldSrc> getScene() const { return m_pSceneInfo; }
    void loadScene(ptr<SSceneInfoGoldSrc> vScene)
    {
        m_pSceneInfo = vScene;
        _loadSceneV(vScene);
    }

    static bool isActorInSight(CActor::Ptr vActor, const SFrustum& vFrustum)
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

    
protected:
    virtual void _loadSceneV(ptr<SSceneInfoGoldSrc> vScene) = 0;

    CCamera::Ptr m_pCamera = nullptr;
    ptr<SSceneInfoGoldSrc> m_pSceneInfo = nullptr;
};
