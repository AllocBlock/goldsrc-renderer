#pragma once
#include "IOImage.h"
#include "Scene.h"

#include <vector>
#include <array>
#include <glm/glm.hpp>

class CLightmap
{
public:
    void clear();
    size_t appendLightmap(ptr<CIOImage> vpImage);
    std::pair<size_t, size_t> getLightmapSize();
    glm::vec2 getAcutalLightmapCoord(size_t vIndex, glm::vec2 vOriginTexCoord);
    ptr<CIOImage> getCombinedLightmap();
private:
    const size_t m_MaxWidth = 0x4000;
    std::vector<ptr<CIOImage>> m_LightmapImages;
    size_t m_TotalWidth = 0, m_TotalHeight = 0;
    size_t m_StartHeight = 0, m_StartWidth = 0, m_CurrentRowHeight = 0;
    std::vector<std::pair<size_t, size_t>> m_Offsets;
    static const size_t m_Channel = 4; // RGBA 4 channels
};

enum class EGoldSrcSpriteType
{
    PARALLEL_UP_RIGHT = 0x00, // ʼ�������������z�����������绨�ݣ�
    FACING_UP_RIGHT, // ��PARALLEL_UP_RIGHT���ƣ����������������������������������λ�þ���
    PARALLEL, // ʼ�����������������Σ�
    ORIENTED, // �̶�����
    PARALLEL_ORIENTED // ʼ��������������Ƿ�������Զ���
};

struct SGoldSrcSprite
{
    glm::vec3 Position;
    glm::vec3 Angle;
    float Scale;
    EGoldSrcSpriteType Type;
    ptr<CIOImage> pImage;
};

struct SSceneInfoGoldSrc
{
    CScene::Ptr pScene = nullptr;
    std::vector<ptr<CIOImage>> TexImageSet;
    std::vector<SGoldSrcSprite> SprSet;

    bool UseSkyBox = false;
    std::array<ptr<CIOImage>, 6> SkyBoxImages;

    // for bsp
    bool UseLightmap = false;
    ptr<CLightmap> pLightmap = nullptr;
};
