#pragma once
#include "Scene.h"
#include "Camera.h"

namespace SceneProbe
{
	bool select(glm::vec2 vNDC, CCamera::Ptr vCamera, SScene::Ptr vScene, ptr<CMeshDataGoldSrc>& voObject, float& voNearestDistance);
};