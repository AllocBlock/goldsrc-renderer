#include "SceneProbe.h"
#include "Maths.h"

#include <glm/matrix.hpp>
#include <optional>

bool __intersectRayActor(glm::vec3 vOrigin, glm::vec3 vDirection, CActor<CMeshDataGoldSrc>::Ptr vActor, float& voNearT)
{
	// check bounding box first
	auto AABB = vActor->getAABB();
	if (!AABB.IsValid) return false;
	float FarT = -INFINITY;
	voNearT = INFINITY;
	if (!Math::intersectRayBoundingBox(vOrigin, vDirection, AABB, voNearT, FarT))
	{
		return false;
	}

	auto MeshData = vActor->getMesh()->getMeshData();
	// intersect each triangle
	_ASSERTE(MeshData.getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_LIST); // FIXME: only support triangle list for now
	auto PosSet = MeshData.getVertexArray();

	bool Hit = false;
	float CurT = 0, u = 0, v = 0;
	for (size_t i = 0; i < PosSet->size(); i += 3)
	{
		if (Math::intersectRayTriangle(vOrigin, vDirection, PosSet->get(i), PosSet->get(i + 1), PosSet->get(i + 2), CurT, u, v))
		{
			Hit = true;
			if (CurT < voNearT) voNearT = CurT;
		}
	}

	return Hit;
}

bool SceneProbe::select(glm::vec2 vNDC, CCamera::Ptr vCamera, CTempScene<CMeshDataGoldSrc>::Ptr vScene, CActor<CMeshDataGoldSrc>::Ptr& voActor, float& voNearestDistance)
{
	if (!vScene || vScene->getActorNum() == 0) return false;
	// normolize screen coordinate
	glm::vec4 ClipSpace = glm::vec4(vNDC, 1.0f, 1.0f);

	// screen coordinate to world coordinate
	// use inverse matrix mult
	glm::mat4 VP = vCamera->getViewProjMat();
	glm::mat4 InversedVP = glm::inverse(VP);
	glm::vec4 WorldPosH = InversedVP * ClipSpace;
	WorldPosH /= WorldPosH.w;
	glm::vec3 WorldPos = glm::vec3(WorldPosH);

	// construct the ray
	glm::vec3 EyePos = vCamera->getPos();
	glm::vec3 Direction = glm::normalize(WorldPos - EyePos);

	float NearestDistance = INFINITY;
    CActor<CMeshDataGoldSrc>::Ptr pNearestActor = nullptr;

	for (size_t i = 0; i < vScene->getActorNum(); ++i)
	{
		auto pActor = vScene->getActor(i);
		std::optional<SAABB> BB = pActor->getAABB();
		if (BB == std::nullopt) continue;
		// find neareset
		float t = 0;
		bool HasIntersection = __intersectRayActor(EyePos, Direction, pActor, t);
		if (HasIntersection && t < NearestDistance)
		{
			NearestDistance = t;
            pNearestActor = pActor;
		}
	}

    if (pNearestActor)
    {
        voNearestDistance = NearestDistance;
        voActor = pNearestActor;
        return true;
    }
    else
        return false;
}
