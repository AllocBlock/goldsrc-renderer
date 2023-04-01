#pragma once
#include "Pointer.h"
#include "Common.h"
#include "Component.h"

#include <string>
#include <glm/glm.hpp>

class CComponentCollider : public IComponent
{
public:
	_DEFINE_PTR(CComponentCollider);

	virtual ~CComponentCollider() = default;
	_DEFINE_GETTER_SETTER(Name, std::string)

private:
	std::string m_Name = "Default Collider";
};

enum class EBasicColliderType
{
	PLANE,
    SPHERE,
	QUAD,
	CUBE
};

class CColliderBasic : public CComponentCollider
{
public:
	_DEFINE_PTR(CColliderBasic);

	CColliderBasic(EBasicColliderType vType = EBasicColliderType::SPHERE): m_Type(vType)
	{}

	_DEFINE_GETTER_SETTER(Type, EBasicColliderType)

private:
	EBasicColliderType m_Type = EBasicColliderType::SPHERE;
};

bool collide(CComponentCollider::CPtr vA, CComponentCollider::CPtr vB, glm::vec3& voPosition, glm::vec3& voNormal, float& voDepth);