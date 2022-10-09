#include "SceneProbe.h"
#include <glm/matrix.hpp>
#include <optional>

bool __intersectRayObject(glm::vec3 vOrigin, glm::vec3 vDirection, ptr<CMeshDataGoldSrc> vObject, float& voNearT)
{
	// check bounding box first
	if (!vObject->getBoundingBox().has_value()) return false;
	float FarT = -INFINITY;
	voNearT = INFINITY;
	if (!Math::intersectRayBoundingBox(vOrigin, vDirection, vObject->getBoundingBox().value(), voNearT, FarT))
	{
		return false;
	}

	// intersect each triangle
	_ASSERTE(vObject->getPrimitiveType() == E3DObjectPrimitiveType::TRIAGNLE_LIST); // FIXME: only support triangle list for now
	auto PosSet = vObject->getVertexArray();

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

bool SceneProbe::select(glm::vec2 vNDC, CCamera::Ptr vCamera, SScene::Ptr vScene, ptr<CMeshDataGoldSrc>& voObject, float& voNearestDistance)
{
    // FIXME: must be some bugs... can select object behind camera
	if (!vScene || vScene->Objects.empty()) return false;
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
    ptr<CMeshDataGoldSrc> pNearestObject = nullptr;

	for (const auto& pObject : vScene->Objects)
	{
		std::optional<Math::S3DBoundingBox> BB = pObject->getBoundingBox();
		if (BB == std::nullopt) continue;
		// TODO: get intersection of ray and bounding box
		// get distance, compare with currert neareset
		// save nearest bounding box
		float t = 0;
		bool HasIntersection = __intersectRayObject(EyePos, Direction, pObject, t);
		if (HasIntersection && t < NearestDistance)
		{
			NearestDistance = t;
            pNearestObject = pObject;
		}
	}

    if (pNearestObject)
    {
        voNearestDistance = NearestDistance;
        voObject = pNearestObject;
        return true;
    }
    else
        return false;
}