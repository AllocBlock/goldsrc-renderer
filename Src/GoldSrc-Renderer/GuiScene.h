#pragma once
#include "Scene.h"
#include "InterfaceUI.h"

class CGuiScene
{
public:
    void setScene(CScene::CPtr vScene) { m_pScene = vScene; }
    void setFocusedActor(wptr<CActor> vActor) { m_pFocusedActor = vActor; }
    void clearFocusedActor() { m_pFocusedActor.reset(); }
    void draw()
    {
        UI::beginWindow(u8"场景");
        if (!m_pScene)
        {
            UI::text(u8"尚未加载场景...");
        }
        else
        {
            if (!m_pFocusedActor.expired())
            {
                auto pActor = m_pFocusedActor.lock();
                if (UI::collapse(u8"当前物体：" + pActor->getName()))
                {
                    __drawActor(pActor, "Focused");
                }
            }

            for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
            {
                auto pActor = m_pScene->getActor(i);
                std::string Id = std::to_string(i);
                if (UI::collapse(pActor->getName() + " - " + Id))
                {
                    __drawActor(pActor, Id);
                }
            }
        }
        UI::endWindow();
    }
private:
    void __drawActor(CActor::Ptr vActor,const std::string& vUniqueId)
    {
        UI::indent();
        bool Visiable = vActor->getVisible();
        UI::toggle(u8"可见##" + vUniqueId, Visiable);
        vActor->setVisible(Visiable);

        auto pTransform = vActor->getTransform();

        glm::vec3 Vec3 = pTransform->getTranslate();
        UI::input(u8"平移##" + vUniqueId, Vec3);
        pTransform->setTranslate(Vec3);

        Vec3 = pTransform->getScale();
        UI::input(u8"缩放##" + vUniqueId, Vec3);
        pTransform->setScale(Vec3);

        Vec3 = pTransform->getRotate().getEulerDegrees();
        UI::input(u8"旋转##" + vUniqueId, Vec3);
        pTransform->setRotate(CRotator::createEulerDegrees(Vec3));

        if (UI::collapse(u8"组件##" + vUniqueId))
        {
            UI::indent();
            for (auto pComp : pTransform->getComponents())
            {
                std::string Id = std::to_string(reinterpret_cast<int64_t>(pComp.get()));
                if (UI::collapse(pComp->getName() + "##" + Id))
                {
                    UI::indent();
                    pComp->renderUI();
                    UI::unindent();
                }
            }
            UI::unindent();
        }
        UI::unindent();
    }

    CScene::CPtr m_pScene = nullptr;
    wptr<CActor> m_pFocusedActor;
};
