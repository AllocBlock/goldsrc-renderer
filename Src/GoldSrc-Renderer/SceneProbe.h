#pragma once
#include "Scene.h"
#include "Camera.h"
#include "3DObject.h"
#include "Maths.h"

namespace SceneProbe
{
	bool select(glm::vec2 vNDC, CCamera::Ptr vCamera, SScene::Ptr vScene, ptr<CMeshDataGoldSrc>& voObject, float& voNearestDistance);
};