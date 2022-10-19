#pragma once
#include "Pointer.h"
#include "Rotator.h"

#include <glm/glm.hpp>

struct STransform
{
	cptr<STransform> pParent = nullptr;
	glm::vec3 Translate = glm::vec3(0.0f);
	CRotator Rotate = CRotator();
	glm::vec3 Scale = glm::vec3(1.0f);

	STransform() = default;

	STransform(glm::vec3 vTranslate, glm::vec3 vRotate, glm::vec3 vScale) :
		Translate(vTranslate), Rotate(vRotate), Scale(vScale)
	{}

	STransform(glm::vec3 vTranslate, glm::vec3 vRotate, float vScale) :
		Translate(vTranslate), Rotate(vRotate), Scale(glm::vec3(vScale))
	{}

	void reset()
	{
		Translate = glm::vec3(0.0f);
		Rotate = CRotator();
		Scale = glm::vec3(1.0f);
	}

	glm::vec3 getAbsoluteTranslate() const;
	CRotator getAbsoluteRotate() const;
	glm::vec3 getAbsoluteScale() const;

	glm::mat4 getRelativeModelMat4() const;
	glm::mat4 getAbsoluteModelMat4() const;
	glm::vec3 applyRelativeOnPoint(glm::vec3 vPoint) const;
	glm::vec3 applyRelativeOnVector(glm::vec3 vVector) const;
	glm::vec3 applyAbsoluteOnPoint(glm::vec3 vPoint) const;
	glm::vec3 applyAbsoluteOnVector(glm::vec3 vVector) const;
private:
	glm::vec4 __applyRelative(glm::vec4 vValue) const;
	glm::vec4 __applyAbsolute(glm::vec4 vValue) const;
};
