#include "SceneProbe.h"
#include <glm/matrix.hpp>
#include <optional>

bool getIntersectionOfRayAndBoundingBox(glm::vec3 vOrigin, glm::vec3 vDirection, S3DBoundingBox vBB, float& voNearT, float& voFarT)
{
    vDirection = glm::normalize(vDirection);

    size_t NumZero = 0;
    if (vDirection.x == 0.0) NumZero++;
    if (vDirection.y == 0.0) NumZero++;
    if (vDirection.z == 0.0) NumZero++;

    if (NumZero == 3)
    {
        throw std::runtime_error(u8"求交的射线方向是零向量");
    }
    else if (NumZero == 2) // 1D
    {
        // one main axis and two zero axes, for reusing 3 conditions on x, y and z axes
        uint8_t AxisMap[3] = { 0, 1, 2 };

        // find main axis and update AxisMap
        for (uint8_t i = 0; i < 3; ++i)
        {
            if (vDirection[i] != 0)
            {
                AxisMap[0] = i;
                AxisMap[i] = 0;
            }
        }

        uint8_t MainAxis = AxisMap[0], ZeroAxis1 = AxisMap[1], ZeroAxis2 = AxisMap[2];

        if (vOrigin[ZeroAxis1] < vBB.Min[ZeroAxis1] || vOrigin[ZeroAxis1] > vBB.Max[ZeroAxis1] || // outside on zero axis 1
            vOrigin[ZeroAxis2] < vBB.Min[ZeroAxis2] || vOrigin[ZeroAxis2] > vBB.Max[ZeroAxis2] || // outside on zero axis 2
            vDirection[MainAxis] > 0 ? vOrigin[MainAxis] > vBB.Max[MainAxis] : vOrigin[MainAxis] < vBB.Min[MainAxis])// outside on main axis 
        {
            return false;
        }
        float T1 = (vBB.Min[MainAxis] - vOrigin[MainAxis]) / vDirection[MainAxis];
        float T2 = (vBB.Max[MainAxis] - vOrigin[MainAxis]) / vDirection[MainAxis];
        float NearT = std::max<float>(T1, T2);
        float FarT = std::min<float>(T1, T2);
        if (NearT < 0) return false;
        voNearT = NearT;
        voFarT = FarT;
        return true;
    }
    else if (NumZero == 1) // 2D
    {
        // two main axes and two zero axis
        uint8_t AxisMap[3] = { 0, 1, 2 };

        // find main axis and update AxisMap
        for (uint8_t i = 0; i < 3; ++i)
        {
            if (vDirection[i] == 0)
            {
                AxisMap[2] = i;
                AxisMap[i] = 2;
            }
        }

        uint8_t MainAxis1 = AxisMap[0], MainAxis2 = AxisMap[1], ZeroAxis = AxisMap[2];

        if (vOrigin[ZeroAxis] < vBB.Min[ZeroAxis] || vOrigin[ZeroAxis] > vBB.Max[ZeroAxis] || // outside on zero axis
            vDirection[MainAxis1] > 0 ? vOrigin[MainAxis1] > vBB.Max[MainAxis1] : vOrigin[MainAxis1] < vBB.Min[MainAxis1] ||
            vDirection[MainAxis2] > 0 ? vOrigin[MainAxis2] > vBB.Max[MainAxis2] : vOrigin[MainAxis2] < vBB.Min[MainAxis2])// outside on main axis
        {
            return false;
        }
        float Axis1T1 = (vBB.Min[MainAxis1] - vOrigin[MainAxis1]) / vDirection[MainAxis1];
        float Axis1T2 = (vBB.Max[MainAxis1] - vOrigin[MainAxis1]) / vDirection[MainAxis1];
        float Axis2T1 = (vBB.Min[MainAxis2] - vOrigin[MainAxis2]) / vDirection[MainAxis2];
        float Axis2T2 = (vBB.Max[MainAxis2] - vOrigin[MainAxis2]) / vDirection[MainAxis2];
        float Axis1NearT = std::min<float>(Axis1T1, Axis1T2);
        float Axis1FarT = std::max<float>(Axis1T1, Axis1T2);
        float Axis2NearT = std::min<float>(Axis2T1, Axis2T2);
        float Axis2FarT = std::max<float>(Axis2T1, Axis2T2);

        float NearT = std::max<float>(Axis1NearT, Axis2NearT);
        float FarT = std::min<float>(Axis1FarT, Axis2FarT);
        if (NearT > FarT)
            return false;
        else
        {
            if (NearT < 0) return false;
            voNearT = NearT;
            voFarT = FarT;
            return true;
        }
    }
    else // 3D
    {
        if (vDirection.x > 0 ? vOrigin.x > vBB.Max.x : vOrigin.x < vBB.Min.x ||
            vDirection.y > 0 ? vOrigin.y > vBB.Max.y : vOrigin.y < vBB.Min.y ||
            vDirection.z > 0 ? vOrigin.z > vBB.Max.z : vOrigin.z < vBB.Min.z)
            // outside on any axis
        {
            return false;
        }
        float Axis1T1 = (vBB.Min.x - vOrigin.x) / vDirection.x;
        float Axis1T2 = (vBB.Max.x - vOrigin.x) / vDirection.x;
        float Axis2T1 = (vBB.Min.y - vOrigin.y) / vDirection.y;
        float Axis2T2 = (vBB.Max.y - vOrigin.y) / vDirection.y;
        float Axis3T1 = (vBB.Min.z - vOrigin.z) / vDirection.z;
        float Axis3T2 = (vBB.Max.z - vOrigin.z) / vDirection.z;
        float Axis1NearT = std::min<float>(Axis1T1, Axis1T2);
        float Axis1FarT = std::max<float>(Axis1T1, Axis1T2);
        float Axis2NearT = std::min<float>(Axis2T1, Axis2T2);
        float Axis2FarT = std::max<float>(Axis2T1, Axis2T2);
        float Axis3NearT = std::min<float>(Axis3T1, Axis3T2);
        float Axis3FarT = std::max<float>(Axis3T1, Axis3T2);

        float NearT = std::max<float>(Axis1NearT, std::max<float>(Axis2NearT, Axis3NearT));
        float FarT = std::min<float>(Axis1FarT, std::min<float>(Axis2FarT, Axis3FarT));
        if (NearT > FarT)
            return false;
        else
        {
            if (NearT < 0) return false;
            voNearT = NearT;
            voFarT = FarT;
            return true;
        }
    }
}

bool SceneProbe::select(glm::vec2 vNDC, CCamera::Ptr vCamera, SScene::Ptr vScene, ptr<C3DObjectGoldSrc>& voObject, float& voNearestDistance, S3DBoundingBox& voBB)
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
	std::optional<S3DBoundingBox> NearestBoundingBox = std::nullopt;
    ptr<C3DObjectGoldSrc> pNearestObject = nullptr;

	for (const auto& pObject : vScene->Objects)
	{
		std::optional<S3DBoundingBox> BB = pObject->getBoundingBox();
		if (BB == std::nullopt) continue;
		// TODO: get intersection of ray and bounding box
		// get distance, compare with currert neareset
		// save nearest bounding box
		float NearT = 0.0f, FarT = 0.0f;
		bool HasIntersection = getIntersectionOfRayAndBoundingBox(EyePos, Direction, BB.value(), NearT, FarT);
		if (HasIntersection && NearT < NearestDistance)
		{
			NearestDistance = NearT;
			NearestBoundingBox = BB;
            pNearestObject = pObject;
		}
	}

    if (NearestBoundingBox.has_value())
    {
        voNearestDistance = NearestDistance;
        voBB = NearestBoundingBox.value();
        voObject = pNearestObject;
        return true;
    }
    else
        return false;
}