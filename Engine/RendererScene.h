#pragma once
#include "RendererBase.h"
#include "Camera.h"
#include "Scene.h"

struct SObjectDataPosition
{
    VkDeviceSize Offset;
    VkDeviceSize Size;
};

class CRendererScene : public CRendererBase
{
public:
    CRendererScene():m_pCamera(std::make_shared<CCamera>()) {};

    std::shared_ptr<CCamera> getCamera() { return m_pCamera; }
    void setCamera(std::shared_ptr<CCamera> vCamera) { m_pCamera = vCamera; }

    std::shared_ptr<SScene> getScene() const { return m_pScene; }
    void loadScene(std::shared_ptr<SScene> vScene) { _loadSceneV(vScene); }

protected:
    virtual void _loadSceneV(std::shared_ptr<SScene> vScene) = 0;

    std::shared_ptr<CCamera> m_pCamera = nullptr;
    std::shared_ptr<SScene> m_pScene = nullptr;
};

