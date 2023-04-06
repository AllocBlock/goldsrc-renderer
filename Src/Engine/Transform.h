#pragma once
#include "Pointer.h"
#include "Rotator.h"
#include "Common.h"
#include "Component.h"
#include "Event.h"

#include <glm/glm.hpp>

class CTransform : public std::enable_shared_from_this<CTransform>
{
	_DEFINE_UPDATE_EVENT(Transform)
public:
	_DEFINE_PTR(CTransform);
	CTransform() = default;

	CTransform(glm::vec3 vTranslate, glm::vec3 vRotate, glm::vec3 vScale) :
		m_Translate(vTranslate), m_Rotate(vRotate), m_Scale(vScale)
	{}

	CTransform(glm::vec3 vTranslate, glm::vec3 vRotate, float vScale) :
		m_Translate(vTranslate), m_Rotate(vRotate), m_Scale(glm::vec3(vScale))
	{}

	CTransform(glm::vec3 vTranslate, CRotator vRotate, glm::vec3 vScale) :
		m_Translate(vTranslate), m_Rotate(vRotate), m_Scale(vScale)
	{}

	CTransform(glm::vec3 vTranslate, CRotator vRotate, float vScale) :
		m_Translate(vTranslate), m_Rotate(vRotate), m_Scale(glm::vec3(vScale))
	{}
	
	_DEFINE_GETTER(Translate, glm::vec3);
	_DEFINE_GETTER(Rotate, CRotator);
	_DEFINE_GETTER(Scale, glm::vec3);

	// TODO: only trigger when really changed
	void setTranslate(const glm::vec3& vTranslate) { m_Translate = vTranslate; m_TransformUpdateEventHandler.trigger(); }
	void setRotate(const CRotator& vRotator) { m_Rotate = vRotator; m_TransformUpdateEventHandler.trigger(); }
	void setRotate(glm::quat vQuat) { m_Rotate = CRotator(vQuat); m_TransformUpdateEventHandler.trigger(); }
	void setScale(const glm::vec3& vScale) { m_Scale = vScale; m_TransformUpdateEventHandler.trigger(); }
	void setScale(float vScale) { m_Scale = glm::vec3(vScale); m_TransformUpdateEventHandler.trigger(); }

	void reset()
	{
		m_Translate = glm::vec3(0.0f);
		m_Rotate = CRotator();
		m_Scale = glm::vec3(1.0f);
		m_TransformUpdateEventHandler.trigger();
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

	CTransform::Ptr getParent() const
	{
		return m_pParent.expired() ? nullptr : m_pParent.lock();
	}

	void addChild(CTransform::Ptr vTransform)
	{
		m_ChildSet.emplace_back(vTransform);
		vTransform->m_pParent = weak_from_this();
	}

	std::vector<IComponent::Ptr> getComponents()
	{
		return m_ComponentSet;
	}

	void addComponent(IComponent::Ptr vComponent)
	{
		m_ComponentSet.emplace_back(vComponent);
		vComponent->__setParent(weak_from_this());
	}

	// return first matched type of component
	template <typename Component_t>
	ptr<Component_t> findComponent() const
	{
		const auto& RxpectTypeId = typeid(Component_t);
	    for (const auto& pComp : m_ComponentSet)
	    {
			if (RxpectTypeId == typeid(*pComp))
				return std::dynamic_pointer_cast<Component_t>(pComp);
	    }
		return nullptr;
	}

private:
	glm::vec4 __applyRelative(glm::vec4 vValue) const;
	glm::vec4 __applyAbsolute(glm::vec4 vValue) const;

	wptr<CTransform> m_pParent;
	std::vector<ptr<CTransform>> m_ChildSet;
	std::vector<ptr<IComponent>> m_ComponentSet;
	glm::vec3 m_Translate = glm::vec3(0.0f);
	CRotator m_Rotate = CRotator();
	glm::vec3 m_Scale = glm::vec3(1.0f);
};