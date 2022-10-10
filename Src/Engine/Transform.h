#pragma once
#include <glm/ext/matrix_transform.hpp>

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
