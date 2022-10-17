#pragma once
#include "Pointer.h"
#include "Common.h"
#include "Transform.h"

#include <string>
#include <glm/glm.hpp>

class ICollider
{
public:
	_DEFINE_PTR(ICollider);

	virtual ~ICollider() = default;
	_DEFINE_GETTER_SETTER(Name, std::string)

private:
	std::string m_Name = "Default Collider";
};

enum class EBasicColliderType
{
	PLANE,
    SPHERE,
	CUBE
};

class CColliderBasic : public ICollider
{
public:
	_DEFINE_PTR(CColliderBasic);

	CColliderBasic(ptr<STransform> vParent = nullptr, EBasicColliderType vType = EBasicColliderType::SPHERE): m_Type(vType)
	{
		m_pTransform->pParent = vParent;
	}

	_DEFINE_GETTER_SETTER(Type, EBasicColliderType)
	_DEFINE_GETTER_POINTER(Transform, ptr<STransform>)

	void setParent(cptr<STransform> vTransform)
	{
		m_pTransform->pParent = vTransform;
	}

private:
	EBasicColliderType m_Type = EBasicColliderType::SPHERE;
	const ptr<STransform> m_pTransform = make<STransform>();;
};

bool collide(ICollider::CPtr vA, ICollider::CPtr vB, glm::vec3& voPosition, glm::vec3& voNormal, float& voDepth);