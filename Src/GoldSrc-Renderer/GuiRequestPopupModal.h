#pragma once
#include "SceneCommon.h"
#include <future>
#include <string>

class CGuiRequestPopupModal
{
public:
    std::future<Scene::ERequestResultState> show(std::string vTitle = u8"δ�ҵ��ļ�", std::string vDescription = "");
    void close(Scene::ERequestResultState vState = Scene::ERequestResultState::CANCEL);
    void draw();
    bool isShow() { return m_IsShow; }
private:
    void __setValue(Scene::ERequestResultState vState);

    bool m_IsShow = false;
    bool m_IsToOpen = false;
    std::string m_Title;
    std::string m_Description;
    std::promise<Scene::ERequestResultState> m_Promise;
};

