#include "Scene.h"

void CScene::addActor(CActor::Ptr vActor)
{
    m_ActorSet.emplace_back(vActor);
}

size_t CScene::getActorNum() const
{ return m_ActorSet.size(); }

CActor::Ptr CScene::getActor(size_t vIndex) const
{
    _ASSERTE(vIndex < m_ActorSet.size());
    return m_ActorSet[vIndex];
}

CActor::Ptr CScene::findActor(std::string vName) const
{
    for (auto pActor : m_ActorSet)
        if (pActor->getName() == vName)
            return pActor;
    return nullptr;
}

void CScene::clear()
{
    m_ActorSet.clear();
}
