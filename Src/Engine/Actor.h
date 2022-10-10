#pragma once
#include "Pointer.h"
#include "Mesh.h"
#include "Common.h"
#include "Transform.h"
#include "PhysicsState.h"

#include <string>

class CActor
{
public:
	_DEFINE_PTR(CActor);

	CActor(std::string vName = "Default Actor"): m_Name(vName)
	{
		m_pPhysicsState->pTargetTransform = m_pTransform; // link
	}

	_DEFINE_GETTER_SETTER(Name, std::string)
	_DEFINE_GETTER_SETTER_POINTER(Mesh, CMesh::CPtr)

	_DEFINE_GETTER_POINTER(Transform, ptr<STransform>)
    _DEFINE_GETTER_POINTER(PhysicsState, ptr<SPhysicsStateRigidBody>)

	glm::vec3 getTranslate() const { return m_pTransform->Translate; }
	glm::vec3 getRotate() const { return m_pTransform->Rotate; }
	glm::vec3 getScale() const { return m_pTransform->Scale; }

	void setTranslate(glm::vec3 v) { m_pTransform->Translate = v; }
	void setRotate(glm::vec3 v) { m_pTransform->Rotate = v; }
	void setScale(glm::vec3 v) { m_pTransform->Scale = v; }
	void setScale(float v) { m_pTransform->Scale = glm::vec3(v); }

	// will not change the transform instance of this actor
	void setTransform(const STransform& vTransform)
	{
		m_pTransform->Translate = vTransform.Translate;
		m_pTransform->Rotate = vTransform.Rotate;
		m_pTransform->Scale = vTransform.Scale;
	}

	// will not change the transform instance of this actor
	void setTransform(ptr<const STransform> vTransform)
	{
		m_pTransform->Translate = vTransform->Translate;
		m_pTransform->Rotate = vTransform->Rotate;
		m_pTransform->Scale = vTransform->Scale;
	}

	void clearMoveState()
	{
		m_pPhysicsState->Velocity = glm::vec3(0.0f);
		// TODO: more state like rotating
	}

private:
	std::string m_Name;
	const ptr<STransform> m_pTransform = make<STransform>();
	CMesh::CPtr m_pMesh = nullptr;
	const ptr<SPhysicsStateRigidBody> m_pPhysicsState = make<SPhysicsStateRigidBody>();;
};
