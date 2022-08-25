#pragma once
#include "SceneCommon.h"
#include <future>
#include <string>

class CGuiRequestPopupModal
{
public:
    std::future<Common::Scene::ERequestResultState> show(std::string vTitle = u8"н╢ур╣╫нд╪Ч", std::string vDescription = "");
    void close(Common::Scene::ERequestResultState vState = Common::Scene::ERequestResultState::CANCEL);
    void draw();
    bool isShow() { return m_IsShow; }
private:
    void __setValue(Common::Scene::ERequestResultState vState);

    bool m_IsShow = false;
    bool m_IsToOpen = false;
    std::string m_Title;
    std::string m_Description;
    std::promise<Common::Scene::ERequestResultState> m_Promise;
};

