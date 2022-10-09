#pragma once
#include "Pointer.h"
#include "Common.h"

#include <string>

class ICollider
{
public:
	_DEFINE_PTR(ICollider);
	_DEFINE_GETTER_SETTER(Name, std::string)

private:
	std::string m_Name = "Default Collider";
};
