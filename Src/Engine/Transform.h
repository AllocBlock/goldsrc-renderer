#pragma once
#include <glm/glm.hpp>

struct STransform
{
	glm::vec3 Translate = glm::vec3(0.0f);
	glm::vec3 Rotate = glm::vec3(0.0f);
	glm::vec3 Scale = glm::vec3(1.0f);

	STransform() = default;

	STransform(glm::vec3 vTranslate, glm::vec3 vRotate, glm::vec3 vScale) :
		Translate(vTranslate), Rotate(vRotate), Scale(vScale)
	{}

	STransform(glm::vec3 vTranslate, glm::vec3 vRotate, float vScale) :
		Translate(vTranslate), Rotate(vRotate), Scale(glm::vec3(vScale))
	{}

	glm::mat4 getModelMat() const;
};
