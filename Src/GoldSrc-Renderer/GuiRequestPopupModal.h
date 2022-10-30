#pragma once
#include "SceneCommon.h"
#include <future>
#include <string>

enum class ERequestAction
{
    MANUAL_FIND,
    RETRY,
    IGNORE_, // IGNORE����Ĭ�Ϻ궨�壬���Լ��˸��»���
    CANCEL
};

class CGuiRequestPopupModal
{
public:
    std::future<ERequestAction> show(std::string vTitle = u8"δ�ҵ��ļ�", std::string vDescription = "");
    void close(ERequestAction vState = ERequestAction::CANCEL);
    void draw();
    bool isShow() { return m_IsShow; }
private:
    void __setValue(ERequestAction vState);

    bool m_IsShow = false;
    bool m_IsToOpen = false;
    std::string m_Title;
    std::string m_Description;
    std::promise<ERequestAction> m_Promise;
};

