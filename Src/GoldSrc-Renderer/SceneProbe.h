#pragma once
#include "SceneInfoGoldSrc.h"
#include "Camera.h"

namespace SceneProbe
{
	bool select(glm::vec2 vNDC, CCamera::Ptr vCamera, CTempScene<CMeshDataGoldSrc>::Ptr vScene, CActor<CMeshDataGoldSrc>::Ptr& voActor, float& voNearestDistance);
};