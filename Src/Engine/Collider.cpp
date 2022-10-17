#include "Collider.h"

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
		glm::vec3 PlaneDirection = glm::normalize(vB->getTransform()->Rotate.getRotateMatrix() * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));

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

bool collide(ICollider::CPtr vA, ICollider::CPtr vB, glm::vec3& voPosition, glm::vec3& voNormal, float& voDepth)
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
	else
		throw std::runtime_error("Unsupported collider type");
}