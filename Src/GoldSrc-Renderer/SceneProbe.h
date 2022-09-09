#pragma once
#include "Scene.h"
#include "Camera.h"
#include "3DObject.h"

namespace SceneProbe
{
	bool select(glm::vec2 vNDC, CCamera::Ptr vCamera, SScene::Ptr vScene, ptr<C3DObjectGoldSrc>& voObject, float& voNearestDistance, S3DBoundingBox& voBB);
};