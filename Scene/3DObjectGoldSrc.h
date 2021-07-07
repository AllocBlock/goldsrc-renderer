#pragma once
#include "3DObject.h"

class C3DObjectGoldSrc : public C3DObject
{
public:
    bool getLightMapEnable() { return m_EnableLightmap; }
    std::shared_ptr<CDataArrayBase<std::optional<size_t>>> getLightmapIndexArray() { return m_LightmapIndexArray; }
    std::shared_ptr<CDataArrayBase<glm::vec2>> getLightmapCoordArray() { return m_LightmapTexCoordArray; }

    void setLightMapEnable(bool vEnable) { m_EnableLightmap = vEnable; }
    void setLightmapIndexArray(std::shared_ptr<CDataArrayBase<std::optional<size_t>>> vLightmapIndexArray) { m_LightmapIndexArray = vLightmapIndexArray; }
    void setLightmapCoordArray(std::shared_ptr<CDataArrayBase<glm::vec2>> vLightmapCoordArray) { m_LightmapTexCoordArray = vLightmapCoordArray; }

protected:
    bool m_EnableLightmap = false;
    std::shared_ptr<CDataArrayBase<std::optional<size_t>>> m_LightmapIndexArray = std::make_shared<CGeneralDataArray<std::optional<size_t>>>();
    std::shared_ptr<CDataArrayBase<glm::vec2>> m_LightmapTexCoordArray = std::make_shared<CGeneralDataArray<glm::vec2>>();
};

