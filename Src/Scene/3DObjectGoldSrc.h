#pragma once
#include "3DObject.h"

class C3DObjectGoldSrc : public C3DObject
{
public:
    bool getLightMapState() { return m_EnableLightmap; }
    IDataArray<std::optional<size_t>>::Ptr getLightmapIndexArray() { return m_LightmapIndexArray; }
    IDataArray<glm::vec2>::Ptr getLightmapCoordArray() { return m_LightmapTexCoordArray; }

    void setLightMapState(bool vEnable) { m_EnableLightmap = vEnable; }
    void setLightmapIndexArray(ptr<IDataArray<std::optional<size_t>>> vLightmapIndexArray) { m_LightmapIndexArray = vLightmapIndexArray; }
    void setLightmapCoordArray(ptr<IDataArray<glm::vec2>> vLightmapCoordArray) { m_LightmapTexCoordArray = vLightmapCoordArray; }

protected:
    bool m_EnableLightmap = false;
    IDataArray<std::optional<size_t>>::Ptr m_LightmapIndexArray = make<CGeneralDataArray<std::optional<size_t>>>();
    IDataArray<glm::vec2>::Ptr m_LightmapTexCoordArray = make<CGeneralDataArray<glm::vec2>>();
};

