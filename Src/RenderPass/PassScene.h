#pragma once
#include "RenderPassSingle.h"
#include "Camera.h"
#include "VertexBuffer.h"
#include "BoundingBox.h"
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
    
protected:
    virtual void _loadSceneV(ptr<SSceneInfoGoldSrc> vScene) = 0;

    CCamera::Ptr m_pCamera = nullptr;
    ptr<SSceneInfoGoldSrc> m_pSceneInfo = nullptr;
};

template <typename PointData_t>
class CRenderPassSceneTyped : public CRenderPassScene
{
public:
    _DEFINE_PTR(CRenderPassSceneTyped<PointData_t>);

    CCamera::Ptr getCamera() { return m_pCamera; }
    void setCamera(CCamera::Ptr vCamera) { m_pCamera = vCamera; }

    ptr<SSceneInfoGoldSrc> getScene() const { return m_pSceneInfo; }

    virtual void _destroyV() override
    {
        destroyAndClear(m_pVertexBuffer);

        CRenderPassScene::_destroyV();
    }
    
    static bool isActorInSight(CActor::Ptr vActor, const SFrustum& vFrustum)
    {
        // AABB frustum culling
        const std::array<glm::vec4, 6>& FrustumPlanes = vFrustum.Planes;
        SAABB BoundingBox = vActor->getAABB();
        if (!BoundingBox.IsValid) return false;

        std::array<glm::vec3, 8> BoundPoints = {};
        for (int i = 0; i < 8; ++i)
        {
            float X = ((i & 1) ? BoundingBox.Min.x : BoundingBox.Max.x);
            float Y = ((i & 2) ? BoundingBox.Min.y : BoundingBox.Max.y);
            float Z = ((i & 4) ? BoundingBox.Min.z : BoundingBox.Max.z);
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
    virtual void _loadSceneV(ptr<SSceneInfoGoldSrc> vScene) override
    {
        m_pDevice->waitUntilIdle();
        destroyAndClear(m_pVertexBuffer);
        m_pVertexBuffer = m_pSceneInfo->pScene->generateVertexBuffer<PointData_t>(m_pDevice);
    }

    void _recordRenderActorCommand(VkCommandBuffer vCommandBuffer, size_t vObjectIndex)
    {
        _ASSERTE(vObjectIndex < m_pVertexBuffer->getSegmentNum());
        const auto& Info = m_pVertexBuffer->getSegmentInfo(vObjectIndex);
        vkCmdDraw(vCommandBuffer, Info.Num, 1, Info.First, 0);
    }
    
    vk::CVertexBuffer::Ptr m_pVertexBuffer = nullptr;
};
