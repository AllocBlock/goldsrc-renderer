#include "Collider.h"
#include "BasicMesh.h"
#include "Common.h"
#include "Transform.h"

bool __collideSphereToSphere(CColliderBasic::CPtr vA, CColliderBasic::CPtr vB, glm::vec3& voPosition, glm::vec3& voNormal, float& voDepth)
{
	if (vA->getType() != EBasicColliderType::SPHERE || vB->getType() != EBasicColliderType::SPHERE)
		throw std::runtime_error("Unsupported collider type");

	glm::vec3 C1 = vA->getTransform()->getAbsoluteTranslate(); float r1 = vA->getTransform()->getAbsoluteScale().x; // FIXME: what if different scale?
	glm::vec3 C2 = vB->getTransform()->getAbsoluteTranslate(); float r2 = vB->getTransform()->getAbsoluteScale().x; // FIXME: what if different scale?

	float d = glm::distance(C1, C2);
	if (d > r1 + r2)
		return false;

	voPosition = C1 + glm::normalize(C2 - C1) * (3 * r1 + r2 - d) * 0.5f; // center of overlapping
	voNormal = glm::normalize(C1 - C2); // direction of force applied on vA
	voDepth = r1 + r2 - d;
	return true;
}

bool __collideSphereToPlane(CColliderBasic::CPtr vA, CColliderBasic::CPtr vB, glm::vec3& voPosition, glm::vec3& voNormal, float& voDepth)
{
	if (vA->getType() == EBasicColliderType::SPHERE && vB->getType() == EBasicColliderType::PLANE)
	{
		glm::vec3 SphereCenter = vA->getTransform()->getAbsoluteTranslate();
	    float SphereRadius = vA->getTransform()->getAbsoluteScale().x; // FIXME: what if different scale?
		glm::vec3 PlaneCenter = vB->getTransform()->getAbsoluteTranslate();
		glm::vec3 PlaneDirection = glm::normalize(vB->getTransform()->getRotate().getRotateMatrix() * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));

		float SignedDistance = glm::dot(SphereCenter - PlaneCenter, PlaneDirection);
		float d = abs(SignedDistance);
		if (d > SphereRadius)
			return false;

		voNormal = PlaneDirection * (SignedDistance > 0 ? 1.0f : -1.0f); // direction of force applied on vA
		voPosition = SphereCenter - voNormal * (d + SphereRadius) * 0.5f; // center of overlapping
		voDepth = d - SphereRadius;
		return true;
	}
	else if (vA->getType() == EBasicColliderType::PLANE && vB->getType() == EBasicColliderType::SPHERE)
	{
		bool Res = __collideSphereToPlane(vB, vA, voPosition, voNormal, voDepth);
		if (Res) voNormal = -voNormal;
		return Res;
	}
	else
		throw std::runtime_error("Unsupported collider type");
}

bool __isPointOnPlane(const glm::vec3& vPlaneCenter, const glm::vec3& vPlaneNormal, const glm::vec3& vPoint)
{
	return glm::abs(glm::dot(vPoint - vPlaneCenter, vPlaneNormal)) < Common::Acc;
}

bool __collideEdgeToPlane(const glm::vec3& vPlaneCenter, const glm::vec3& vPlaneNormal, const glm::vec3& vEdgeStart, const glm::vec3& vEdgeEnd, glm::vec3& voPosition, glm::vec3& voNormal, float& voDepth)
{
	glm::vec3 D = glm::normalize(vEdgeEnd - vEdgeStart);
	const float MaxT = glm::distance(vEdgeStart, vEdgeEnd);
	if (__isPointOnPlane(vPlaneCenter, vPlaneNormal, vEdgeStart) && __isPointOnPlane(vPlaneCenter, vPlaneNormal, vEdgeEnd)) // inside
	{
		voPosition = (vEdgeStart + vEdgeEnd) * 0.5f;
		voNormal = vPlaneNormal;
		voDepth = glm::abs(glm::dot(vPlaneNormal, voPosition - vPlaneCenter));
		return true;
	}

	float t = glm::dot(vPlaneNormal, vPlaneCenter - vEdgeStart) / glm::dot(vPlaneNormal, D);
	if (t < -Common::Acc || t > MaxT + Common::Acc) return false;

	voPosition = vEdgeStart + t * D;
	voNormal = vPlaneNormal;
	voDepth = (MaxT - t) * glm::abs(glm::dot(vPlaneNormal, D));

	return true;
}

bool __collideQuadToPlane(CColliderBasic::CPtr vA, CColliderBasic::CPtr vB, glm::vec3& voPosition, glm::vec3& voNormal, float& voDepth)
{
	if (vA->getType() == EBasicColliderType::QUAD && vB->getType() == EBasicColliderType::PLANE)
	{
		const glm::vec3 PlaneCenter = vB->getTransform()->getTranslate();
		const glm::vec3 PlaneNormal = vB->getTransform()->getRotate().getOrientation();

		glm::vec3 SumPosition = glm::vec3(0.0f);
		glm::vec3 SumNormal = glm::vec3(0.0f);
		float SumDepth = 0.0f;
		int Num = 0;

		glm::vec3 Position, Normal;
		float Depth;

		auto QuadEdgeSet = BasicMesh::getUnitQuadEdgeSet();
		for (size_t i = 0; i < QuadEdgeSet.size(); i += 2)
		{
			glm::vec3 Start = vA->getTransform()->applyAbsoluteOnPoint(QuadEdgeSet[i].Pos);
			glm::vec3 End = vA->getTransform()->applyAbsoluteOnPoint(QuadEdgeSet[i + 1].Pos);
			if (__collideEdgeToPlane(PlaneCenter, PlaneNormal, Start, End, Position, Normal, Depth))
			{
				SumPosition += Position;
				SumNormal += Normal;
				SumDepth += Depth;
				Num++;
			}
		}

		if (Num > 0)
		{
			voPosition = SumPosition / static_cast<float>(Num);
			voNormal = SumNormal / static_cast<float>(Num);
			voDepth = SumDepth / static_cast<float>(Num);
			return true;
		}
		else return false;
	}
	else if (vA->getType() == EBasicColliderType::PLANE && vB->getType() == EBasicColliderType::QUAD)
	{
		bool Res = __collideQuadToPlane(vB, vA, voPosition, voNormal, voDepth);
		if (Res) voNormal = -voNormal;
		return Res;
	}
	else
		throw std::runtime_error("Unsupported collider type");
}

bool collide(CComponentCollider::CPtr vA, CComponentCollider::CPtr vB, glm::vec3& voPosition, glm::vec3& voNormal, float& voDepth)
{
	if (!vA || !vB) return false;

	auto pA = std::dynamic_pointer_cast<const CColliderBasic>(vA);
	auto pB = std::dynamic_pointer_cast<const CColliderBasic>(vB);

	if (!pA || !pB)
		throw std::runtime_error("Unsupported collider type");

	if (pA->getType() == EBasicColliderType::SPHERE && pB->getType() == EBasicColliderType::SPHERE)
		return __collideSphereToSphere(pA, pB, voPosition, voNormal, voDepth);
	else if ((pA->getType() == EBasicColliderType::SPHERE && pB->getType() == EBasicColliderType::PLANE) ||
		(pA->getType() == EBasicColliderType::PLANE && pB->getType() == EBasicColliderType::SPHERE))
		return __collideSphereToPlane(pA, pB, voPosition, voNormal, voDepth);
	else if ((pA->getType() == EBasicColliderType::QUAD && pB->getType() == EBasicColliderType::PLANE) ||
		(pA->getType() == EBasicColliderType::PLANE && pB->getType() == EBasicColliderType::QUAD))
		return __collideQuadToPlane(pA, pB, voPosition, voNormal, voDepth);
	else
		throw std::runtime_error("Unsupported collider type");
}