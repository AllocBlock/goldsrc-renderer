#pragma once
#include "Component.h"
#include "Mesh.h"

class CComponentMeshRenderer : public IComponent
{
public:
    _DEFINE_GETTER_SETTER_POINTER(Mesh, ptr<CMesh>);
    bool hasMesh() const { return m_pMesh != nullptr; }

    virtual SAABB getAABBV() const override
    {
        return m_pMesh->getAABB();
    }

protected:
    virtual std::string _getNameV() const override { return "Mesh Renderer"; }

private:
    ptr<CMesh> m_pMesh = nullptr;
};
