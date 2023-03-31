#pragma once
#include "SceneInfoGoldSrc.h"
#include "Camera.h"

namespace SceneProbe
{
	bool select(glm::vec2 vNDC, CCamera::Ptr vCamera, CScene<CMeshDataGoldSrc>::Ptr vScene, CActor<CMeshDataGoldSrc>::Ptr& voActor, glm::vec3& voNearestIntersection);
};