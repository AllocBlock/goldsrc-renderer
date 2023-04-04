#pragma once
#include "Scene.h"
#include "InterfaceUI.h"

class CGuiScene
{
public:
    void setScene(CScene::CPtr vScene) { m_pScene = vScene; }
    void draw()
    {
        UI::beginWindow(u8"场景");
        if (!m_pScene)
        {
            UI::text(u8"尚未加载场景...");
        }
        else
        {
            for (size_t i = 0; i < m_pScene->getActorNum(); ++i)
            {
                auto pActor = m_pScene->getActor(i);
                if (UI::collapse(pActor->getName() + " - " + std::to_string(i)))
                {
                    UI::indent(20.0f);
                    bool Visiable = pActor->getVisible();
                    UI::toggle(u8"可见##" + std::to_string(i), Visiable);
                    pActor->setVisible(Visiable);

                    auto pTransform = pActor->getTransform();

                    glm::vec3 Vec3 = pTransform->getTranslate();
                    UI::input(u8"平移##" + std::to_string(i), Vec3);
                    pTransform->setTranslate(Vec3);

                    Vec3 = pTransform->getScale();
                    UI::input(u8"缩放##" + std::to_string(i), Vec3);
                    pTransform->setScale(Vec3);

                    Vec3 = pTransform->getRotate().getEulerDegrees();
                    UI::input(u8"旋转##" + std::to_string(i), Vec3);
                    pTransform->setRotate(CRotator::createEulerDegrees(Vec3));

                    if (UI::collapse(u8"组件##" + std::to_string(i)))
                    {
                        UI::indent(20.0f);
                        for (auto pComp : pTransform->getComponents())
                        {
                            UI::text(pComp->getName());
                        }
                        UI::unindent();
                    }
                    UI::unindent();
                }
            }
        }
        UI::endWindow();
    }
private:
    CScene::CPtr m_pScene = nullptr;
};
