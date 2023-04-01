#pragma once
#include "Pointer.h"
#include "Common.h"
#include "Transform.h"

#include <string>
#include <set>

class CActor
{
public:
	_DEFINE_PTR(CActor);

	CActor(std::string vName = "Default Actor"): m_Name(vName)
	{
		m_pTransform = make<CTransform>();
	}

	_DEFINE_GETTER_SETTER(Name, std::string)
	_DEFINE_GETTER_POINTER(Transform, ptr<CTransform>)

	void addTag(const std::string& vTag) { m_TagSet.insert(vTag); }
	bool hasTag(const std::string& vTag) { return m_TagSet.count(vTag) > 0; }
	void removeTag(const std::string& vTag) { if (hasTag(vTag)) m_TagSet.erase(vTag); }
	void clearAllTag() { m_TagSet.clear(); }

private:
	std::string m_Name;
	CTransform::Ptr m_pTransform;
	std::set<std::string> m_TagSet;
};
