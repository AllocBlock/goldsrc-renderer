#include "IOGoldSrcBsp.h"
#include "IOLog.h"

#include <fstream>

bool CIOGoldSrcBsp::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File;
    File.open(vFilePath.string(), std::ios::in | std::ios::binary);
    if (!File.is_open())
    {
        GlobalLogger::logStream() << u8"¶ÁÈ¡ÎÄ¼þ " << vFilePath << u8" Ê§°Ü";
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

template <typename T>
std::vector<T> readArray(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    vFile.seekg(vOffset, std::ios_base::beg);

    _ASSERTE(vSize % sizeof(T) == 0);
    size_t NumT = vSize / sizeof(T);
    std::vector<T> Result;
    Result.resize(NumT);
    vFile.read(reinterpret_cast<char*>(Result.data()), vSize);
    return std::move(Result);
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
}

void SLumpPlane::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Planes = readArray<SBspPlane>(vFile, vOffset, vSize);
}

void SBspTexture::read(std::ifstream& vFile, uint64_t vOffset)
{
    vFile.seekg(vOffset, std::ios_base::beg);

    char TempNameBuffer[g_MaxNameLength];
    vFile.read(TempNameBuffer, g_MaxNameLength);
    Name = TempNameBuffer;
    vFile.read(reinterpret_cast<char*>(&Width), sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(&Height), sizeof(uint32_t));
    vFile.read(reinterpret_cast<char*>(Offsets), g_BspMipmapLevel * sizeof(uint32_t));

    if (pData) delete[] pData;
    IsDataInBsp = (Offsets[0] > 0);
    if (IsDataInBsp)
    {
        size_t DataSize = static_cast<size_t>(Width) * Height * 4;
        pData = new char[DataSize];
        vFile.read(static_cast<char*>(pData), vOffset + Offsets[0]);
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
    Vertices = readArray<SVec3>(vFile, vOffset, vSize);
}

void SLumpVisibility::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Vis = readArray<uint8_t>(vFile, vOffset, vSize);
}

void SLumpNode::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Nodes = readArray<SBspNode>(vFile, vOffset, vSize);
}

void SLumpTexInfo::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    TexInfos = readArray<SBspTexInfo>(vFile, vOffset, vSize);
}

void SLumpFace::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Faces = readArray<SBspFace>(vFile, vOffset, vSize);
}

void SLumpLighting::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Lightmaps = readArray<SLightmap>(vFile, vOffset, vSize);
}

void SLumpClipNode::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    ClipNodes = readArray<SBspClipNode>(vFile, vOffset, vSize);
}

void SLumpLeaf::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Leaves = readArray<SBspLeaf>(vFile, vOffset, vSize);
}

void SLumpMarkSurface::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    FaceIndices = readArray<uint16_t>(vFile, vOffset, vSize);
}

void SLumpEdge::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Edges = readArray<SBspEdge>(vFile, vOffset, vSize);
}

void SLumpSurfedge::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Surfedges = readArray<int32_t>(vFile, vOffset, vSize);
}

void SLumpModel::read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize)
{
    Models = readArray<SBspModel>(vFile, vOffset, vSize);
}

glm::vec3 SVec3::glmVec3() const
{
    return glm::vec3(X, Y, Z);
}

bool SBspTexture::getRawRGBAPixels(void* vopData) const
{
    if (!IsDataInBsp) return false;
    size_t DataSize = static_cast<size_t>(Width) * Height * 4;
    char* pIter = static_cast<char*>(pData);
    std::copy(pIter, pIter + DataSize, static_cast<char*>(vopData));
    return true;
}

glm::vec2 SBspTexInfo::getTexCoord(glm::vec3 vVertex, size_t vTexWidth, size_t vTexHeight) const
{
    glm::vec2 TexCoord;
    TexCoord.x = (glm::dot(vVertex, TextureDirectionU.glmVec3()) + TextureOffsetU) / vTexWidth;
    TexCoord.y = (glm::dot(vVertex, TextureDirectionV.glmVec3()) + TextureOffsetV) / vTexHeight;
    return TexCoord;
}