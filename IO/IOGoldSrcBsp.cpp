#include "IOCommon.h"
#include "IOGoldSrcBsp.h"

#include <fstream>

bool CIOGoldSrcBsp::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File;
    File.open(vFilePath.string(), std::ios::in | std::ios::binary);
    if (!File.is_open())
    {
        globalLog(u8"打开文件 [" + vFilePath.u8string() + u8"] 失败，无权限或文件不存在");
        return false;
    }
    m_Header.read(File);

    m_Lumps.m_LumpEntity.read(      File, m_Header.LumpInfos[0].Offset,  m_Header.LumpInfos[0].Size);
    m_Lumps.m_LumpPlane.read(       File, m_Header.LumpInfos[1].Offset,  m_Header.LumpInfos[1].Size);
    m_Lumps.m_LumpTexture.read(     File, m_Header.LumpInfos[2].Offset);
    m_Lumps.m_LumpVertex.read(      File, m_Header.LumpInfos[3].Offset,  m_Header.LumpInfos[3].Size);
    m_Lumps.m_LumpVisibility.read(  File, m_Header.LumpInfos[4].Offset,  m_Header.LumpInfos[4].Size);
    m_Lumps.m_LumpNode.read(        File, m_Header.LumpInfos[5].Offset,  m_Header.LumpInfos[5].Size);
    m_Lumps.m_LumpTexInfo.read(     File, m_Header.LumpInfos[6].Offset,  m_Header.LumpInfos[6].Size);
    m_Lumps.m_LumpFace.read(        File, m_Header.LumpInfos[7].Offset,  m_Header.LumpInfos[7].Size);
    m_Lumps.m_LumpLighting.read(    File, m_Header.LumpInfos[8].Offset,  m_Header.LumpInfos[8].Size);
    m_Lumps.m_LumpClipNode.read(    File, m_Header.LumpInfos[9].Offset,  m_Header.LumpInfos[9].Size);
    m_Lumps.m_LumpLeaf.read(        File, m_Header.LumpInfos[10].Offset, m_Header.LumpInfos[10].Size);
    m_Lumps.m_LumpMarkSurface.read( File, m_Header.LumpInfos[11].Offset, m_Header.LumpInfos[11].Size);
    m_Lumps.m_LumpEdge.read(        File, m_Header.LumpInfos[12].Offset, m_Header.LumpInfos[12].Size);
    m_Lumps.m_LumpSurfedge.read(    File, m_Header.LumpInfos[13].Offset, m_Header.LumpInfos[13].Size);
    m_Lumps.m_LumpModel.read(       File, m_Header.LumpInfos[14].Offset, m_Header.LumpInfos[14].Size);

    return true;
}

void SBspHeader::read(std::ifstream& vFile)
{
    vFile.seekg(0, std::ios_base::beg);

    vFile.read(reinterpret_cast<char*>(&Version), sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(LumpInfos), sizeof(LumpInfos));
    _ASSERTE(Version >= 30);
}

void SLumpEntity::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    vFile.seekg(vOffset, std::ios_base::beg);

    Data.resize(vSize);
    vFile.read(Data.data(), vSize);

    CIOGoldSrcMap Map;
    Map.readFromString(Data);
    Entities = Map.getEntities();
    WadPaths = Map.getWadPaths();
    SkyBoxPrefix = Map.getSkyBoxPrefix();
}

void SLumpPlane::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Planes = IOCommon::readArray<SBspPlane>(vFile, vOffset, vSize);
}

void SBspTexture::read(std::ifstream& vFile, uint64_t vOffset)
{
    vFile.seekg(vOffset, std::ios_base::beg);

    char TempNameBuffer[BSP_MAX_NAME_LENGTH];
    vFile.read(TempNameBuffer, BSP_MAX_NAME_LENGTH);
    Name = TempNameBuffer;
    vFile.read(reinterpret_cast<char*>(&Width), sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&Height), sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(Offsets), BSP_MIPMAP_LEVEL * sizeof(uint32_t));

    if (pIndices) delete[] pIndices;
    IsDataInBsp = (Offsets[0] > 0);
    if (IsDataInBsp)
    {
        size_t DataSize = static_cast<size_t>(Width) * Height;
        size_t PalleteOffset = Offsets[3] + DataSize / 64;
        uint16_t PalleteSize = 0; // should be 256 in file
        uint16_t PalletePadding = 0; // should be 0
        vFile.seekg(vOffset + PalleteOffset, std::ios_base::beg);
        vFile.read(reinterpret_cast<char*>(&PalleteSize), sizeof(uint16_t));
        vFile.read(reinterpret_cast<char*>(Palette), sizeof(Palette));
        vFile.read(reinterpret_cast<char*>(&PalletePadding), sizeof(uint16_t));

        pIndices = new uint8_t[DataSize];
        vFile.seekg(vOffset + Offsets[0], std::ios_base::beg);
        vFile.read(reinterpret_cast<char*>(pIndices), DataSize); // ignore high level mipmap
    }
}

void SLumpTexture::read(std::ifstream& vFile, uint64_t vOffset)
{
    vFile.seekg(vOffset, std::ios_base::beg);

    vFile.read(reinterpret_cast<char*>(&NumTexture), sizeof(uint32_t));

    TexOffsets.resize(NumTexture);
    vFile.read(reinterpret_cast<char*>(TexOffsets.data()), NumTexture * sizeof(int32_t));

    Textures.resize(NumTexture);
    for (uint32_t i = 0; i < NumTexture; ++i)
    {
        Textures[i].read(vFile, vOffset + TexOffsets[i]);
    }
}

void SLumpVertex::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Vertices = IOCommon::readArray<IOCommon::SGoldSrcVec3>(vFile, vOffset, vSize);
}

void SLumpVisibility::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Vis = IOCommon::readArray<uint8_t>(vFile, vOffset, vSize);
}

void SLumpNode::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Nodes = IOCommon::readArray<SBspNode>(vFile, vOffset, vSize);
}

void SLumpTexInfo::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    TexInfos = IOCommon::readArray<SBspTexInfo>(vFile, vOffset, vSize);
}

void SLumpFace::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Faces = IOCommon::readArray<SBspFace>(vFile, vOffset, vSize);
}

void SLumpLighting::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Lightmaps = IOCommon::readArray<SBspLightmap>(vFile, vOffset, vSize);
}

void SLumpClipNode::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    ClipNodes = IOCommon::readArray<SBspClipNode>(vFile, vOffset, vSize);
}

void SLumpLeaf::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Leaves = IOCommon::readArray<SBspLeaf>(vFile, vOffset, vSize);
}

void SLumpMarkSurface::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    FaceIndices = IOCommon::readArray<uint16_t>(vFile, vOffset, vSize);
}

void SLumpEdge::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Edges = IOCommon::readArray<SBspEdge>(vFile, vOffset, vSize);
}

void SLumpSurfedge::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Surfedges = IOCommon::readArray<int32_t>(vFile, vOffset, vSize);
}

void SLumpModel::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Models = IOCommon::readArray<SBspModel>(vFile, vOffset, vSize);
}

bool SBspTexture::getRawRGBAPixels(void* vopData) const
{
    if (!IsDataInBsp) return false;
    bool HasAlphaIndex = false;
    if (Name[0] == '{')
    { 
        HasAlphaIndex = true;
    }

    size_t Resolution = static_cast<size_t>(Width * Height);
    char* pIter = reinterpret_cast<char*>(vopData);
    for (size_t i = 0; i < Resolution; ++i)
    {
        if (HasAlphaIndex && false)
        {
            pIter[i * 4] = 0x00;
            pIter[i * 4 + 1] = 0x00;
            pIter[i * 4 + 2] = 0x00;
            pIter[i * 4 + 3] = 0x00;
        }
        else
        {
            const IOCommon::SGoldSrcColor& Color = Palette[pIndices[i]];
            pIter[i * 4] = Color.R;
            pIter[i * 4 + 1] = Color.G;
            pIter[i * 4 + 2] = Color.B;
            pIter[i * 4 + 3] = static_cast<uint8_t>(255);
        }
    }
    return true;
}

glm::vec2 SBspTexInfo::getTexCoord(glm::vec3 vVertex) const
{
    glm::vec2 TexCoord;
    TexCoord.x = (glm::dot(vVertex, TextureDirectionU.glmVec3()) + TextureOffsetU);
    TexCoord.y = (glm::dot(vVertex, TextureDirectionV.glmVec3()) + TextureOffsetV);
    return TexCoord;
}

glm::vec2 SBspTexInfo::getNormalizedTexCoord(glm::vec3 vVertex, size_t vTexWidth, size_t vTexHeight) const
{
    glm::vec2 TexCoord;
    TexCoord.x = (glm::dot(vVertex, TextureDirectionU.glmVec3()) + TextureOffsetU) / vTexWidth;
    TexCoord.y = (glm::dot(vVertex, TextureDirectionV.glmVec3()) + TextureOffsetV) / vTexHeight;
    return TexCoord;
}

void SLumpLighting::getRawRGBAPixels(size_t vLightmapOffset, size_t vLightmapLength, void* vopData) const
{
    _ASSERTE(vLightmapOffset + vLightmapLength <= Lightmaps.size());
    uint8_t* pIter = reinterpret_cast<uint8_t*>(vopData);
    for (size_t i = 0; i < vLightmapLength; ++i)
    {
        pIter[i * 4] = Lightmaps[vLightmapOffset + i].R;
        pIter[i * 4 + 1] = Lightmaps[vLightmapOffset + i].G;
        pIter[i * 4 + 2] = Lightmaps[vLightmapOffset + i].B;
        pIter[i * 4 + 3] = static_cast<uint8_t>(255);
    }
}