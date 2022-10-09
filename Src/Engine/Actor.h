#pragma once
#include "Pointer.h"
#include "Mesh.h"
#include "Collider.h"
#include "Common.h"

#include <string>
#include <glm/ext/matrix_transform.hpp>

struct STransform
{
	glm::vec3 Translate = glm::vec3(0.0f);
	glm::vec3 Rotate = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);

	glm::mat4 getModelMat() const
	{
		glm::mat4 Result = glm::mat4(1.0f);
		Result = glm::translate(Result, Translate); // be care that the api applys reversely, translate is the last
		Result = glm::rotate(Result, glm::radians(Rotate.x), glm::vec3(1.0, 0.0, 0.0));
		Result = glm::rotate(Result, glm::radians(Rotate.y), glm::vec3(0.0, 1.0, 0.0));
		Result = glm::rotate(Result, glm::radians(Rotate.z), glm::vec3(0.0, 0.0, 1.0));
		Result = glm::scale(Result, Scale);
		return Result;
	}
};

class CActor
{
public:
	_DEFINE_PTR(CActor);

	CActor(std::string vName = "Default Actor"): m_Name(vName) {}

	_DEFINE_GETTER_SETTER(Name, std::string)
	_DEFINE_GETTER_SETTER(Transform, STransform)
	_DEFINE_GETTER_SETTER_POINTER(Mesh, CMesh::CPtr)
	_DEFINE_GETTER_SETTER_POINTER(Collider, ICollider::CPtr)

private:
	std::string m_Name;
	STransform m_Transform;
	CMesh::CPtr m_pMesh = nullptr;
	ICollider::CPtr m_pCollider = nullptr;
};
