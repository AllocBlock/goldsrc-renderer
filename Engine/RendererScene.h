#pragma once
#include "RenderPass.h"
#include "Camera.h"
#include "Scene.h"

struct SObjectDataPosition
{
    VkDeviceSize Offset;
    VkDeviceSize Size;
};

class CRendererScene : public IRenderPass
{
public:
    CRendererScene():m_pCamera(make<CCamera>()) {};

    ptr<CCamera> getCamera() { return m_pCamera; }
    void setCamera(ptr<CCamera> vCamera) { m_pCamera = vCamera; }

    ptr<SScene> getScene() const { return m_pScene; }
    void loadScene(ptr<SScene> vScene) { _loadSceneV(vScene); }

protected:
    virtual void _loadSceneV(ptr<SScene> vScene) = 0;

    ptr<CCamera> m_pCamera = nullptr;
    ptr<SScene> m_pScene = nullptr;
};

