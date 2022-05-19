#pragma once
#include "IRenderPass.h"
#include "Camera.h"
#include "Scene.h"

struct SObjectDataPosition
{
    VkDeviceSize Offset;
    VkDeviceSize Size;
};

class CSceneRenderPass : public vk::IRenderPass
{
public:
    ptr<CCamera> getCamera() { return m_pCamera; }
    void setCamera(ptr<CCamera> vCamera) { m_pCamera = vCamera; }

    ptr<SScene> getScene() const { return m_pScene; }
    void loadScene(ptr<SScene> vScene) 
    { 
        m_pScene = vScene;
        m_ObjectDataPositionSet.resize(m_pScene->Objects.size());

        size_t IndexOffset = 0;
        size_t VertexOffset = 0;
        for (size_t i = 0; i < m_pScene->Objects.size(); ++i)
        {
            ptr<C3DObjectGoldSrc> pObject = m_pScene->Objects[i];
            if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_LIST)
            {
                m_ObjectDataPositionSet[i].Offset = VertexOffset;
                m_ObjectDataPositionSet[i].Size = pObject->getVertexArray()->size();
                VertexOffset += m_ObjectDataPositionSet[i].Size;
            }
            else
                throw std::runtime_error(u8"物体类型错误");
        }
        _loadSceneV(vScene); 
    }

protected:
    virtual void _loadSceneV(ptr<SScene> vScene) = 0;
    void _recordObjectRenderCommand(VkCommandBuffer vCommandBuffer, size_t vObjectIndex)
    {
        _ASSERTE(vObjectIndex >= 0 && vObjectIndex < m_pScene->Objects.size());
        ptr<C3DObjectGoldSrc> pObject = m_pScene->Objects[vObjectIndex];
        SObjectDataPosition DataPosition = m_ObjectDataPositionSet[vObjectIndex];

        uint32_t Size = static_cast<uint32_t>(DataPosition.Size);
        uint32_t Offset = static_cast<uint32_t>(DataPosition.Offset);
        if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::INDEXED_TRIAGNLE_LIST)
            vkCmdDrawIndexed(vCommandBuffer, Size, 1, Offset, 0, 0);
        else if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_LIST)
            vkCmdDraw(vCommandBuffer, Size, 1, Offset, 0);
        else if (pObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_STRIP_LIST)
            vkCmdDraw(vCommandBuffer, Size, 1, Offset, 0);
        else
            throw std::runtime_error(u8"物体类型错误");
    }

    static bool _isObjectInSight(ptr<C3DObject> vpObject, const SFrustum& vFrustum)
    {
        // AABB frustum culling
        const std::array<glm::vec4, 6>& FrustumPlanes = vFrustum.Planes;
        std::optional<S3DBoundingBox> BoundingBox = vpObject->getBoundingBox();
        if (BoundingBox == std::nullopt) return false;

        std::array<glm::vec3, 8> BoundPoints = {};
        for (int i = 0; i < 8; ++i)
        {
            float X = ((i & 1) ? BoundingBox.value().Min.x : BoundingBox.value().Max.x);
            float Y = ((i & 2) ? BoundingBox.value().Min.y : BoundingBox.value().Max.y);
            float Z = ((i & 4) ? BoundingBox.value().Min.z : BoundingBox.value().Max.z);
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

    ptr<CCamera> m_pCamera = nullptr;
    ptr<SScene> m_pScene = nullptr;
    std::vector<SObjectDataPosition> m_ObjectDataPositionSet;
};

