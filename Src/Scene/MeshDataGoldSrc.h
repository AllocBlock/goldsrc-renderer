#pragma once
#include "Mesh.h"

class CMeshDataGoldSrc : public CMeshDataGeneral
{
public:
    _DEFINE_GETTER_SETTER(EnableLightmap, bool)
    _DEFINE_GETTER_SETTER_POINTER(LightmapIndexArray, IDataArray<uint32_t>::Ptr)
    _DEFINE_GETTER_SETTER_POINTER(LightmapTexCoordArray, IDataArray<glm::vec2>::Ptr)

    static const uint32_t InvalidLightmapIndex;
        
protected:
    bool m_EnableLightmap = false;
    IDataArray<uint32_t>::Ptr m_pLightmapIndexArray = make<CGeneralDataArray<uint32_t>>();
    IDataArray<glm::vec2>::Ptr m_pLightmapTexCoordArray = make<CGeneralDataArray<glm::vec2>>();
};
