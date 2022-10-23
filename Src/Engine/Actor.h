#pragma once
#include "Pointer.h"
#include "Mesh.h"
#include "Common.h"
#include "Transform.h"
#include "PhysicsState.h"

#include <string>
#include <set>

template <typename MeshData_t = CMeshDataGeneral>
class CActor
{
public:
	_DEFINE_PTR(CActor);

	CActor(std::string vName = "Default Actor"): m_Name(vName)
	{
		m_pPhysicsState->pTargetTransform = m_pTransform; // link
	}

	_DEFINE_GETTER_SETTER(Name, std::string)
	_DEFINE_GETTER_SETTER_POINTER(Mesh, cptr<CMesh<MeshData_t>>)

	_DEFINE_GETTER_POINTER(Transform, ptr<STransform>)
    _DEFINE_GETTER_POINTER(PhysicsState, ptr<SPhysicsStateRigidBody>)

	glm::vec3 getTranslate() const { return m_pTransform->Translate; }
	const CRotator& getRotate() const { return m_pTransform->Rotate; }
	glm::vec3 getScale() const { return m_pTransform->Scale; }

	void resetTransform() { m_pTransform->reset(); }
	void setTranslate(glm::vec3 v) { m_pTransform->Translate = v; }
	void setRotate(const CRotator& v) { m_pTransform->Rotate = v; }
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
		m_pPhysicsState->AngularVelocity = glm::vec3(0.0f);
		m_pPhysicsState->clearForce();
		m_pPhysicsState->clearAlpha();
	}

	void bakeTransform()
	{
		m_pMesh = Mesh::bakeTransform(m_pMesh, m_pTransform);
		m_pTransform->reset();
	}

	void addTag(const std::string& vTag) { m_TagSet.insert(vTag); }
	bool hasTag(const std::string& vTag) { return m_TagSet.count(vTag) > 0; }
	void removeTag(const std::string& vTag) { if (hasTag(vTag)) m_TagSet.erase(vTag); }
	void clearAllTag() { m_TagSet.clear(); }

	SAABB getAABB() const
	{
		return m_pMesh->getAABB().transform(m_pTransform);
	}

private:
	std::string m_Name;
	const ptr<STransform> m_pTransform = make<STransform>();
	cptr<CMesh<MeshData_t>> m_pMesh = nullptr;
	const ptr<SPhysicsStateRigidBody> m_pPhysicsState = make<SPhysicsStateRigidBody>();

	std::set<std::string> m_TagSet;
};
