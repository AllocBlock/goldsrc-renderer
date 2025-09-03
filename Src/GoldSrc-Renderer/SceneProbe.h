#pragma once
#include "SceneInfo.h"
#include "Camera.h"

namespace SceneProbe
{
	bool select(glm::vec2 vNDC, sptr<CCamera> vCamera, sptr<CScene> vScene, sptr<CActor>& voActor, glm::vec3& voNearestIntersection);
};