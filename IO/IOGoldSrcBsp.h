#pragma once

#include "IOBase.h"
#include "IOGoldSrcMap.h"
#include <vector>

const size_t g_MaxNameLength = 16;
const size_t g_BspMipmapLevel = 4;
const size_t g_MaxHull = 4;

struct SVec3
{
    float X, Y, Z;

    glm::vec3 glmVec3() const;
};

enum class ELumpType
{
    ENTITIES = 0,
    PLANES,
    TEXTURES,
    VERTICES,
    VISIBILITY,
    NODES,
    TEXINFO,
    FACES,
    LIGHTING,
    CLIPNODES,
    LEAVES,
    MARKSURFACES,
    EDGES,
    SURFEDGES,
    MODELS,
    NUM_LUMP_TYPE
};

struct SBspLumpInfo
{
    int32_t Offset = 0;
    int32_t Size = 0;
};

struct SBspHeader
{
    int32_t Version = 0;
    SBspLumpInfo LumpInfos[static_cast<size_t>(ELumpType::NUM_LUMP_TYPE)];

    void read(std::ifstream& vFile);
};

//#define MAX_MAP_HULLS        4
//
//#define MAX_MAP_MODELS       400
//#define MAX_MAP_BRUSHES      4096
//#define MAX_MAP_ENTITIES     1024
//#define MAX_MAP_ENTSTRING    (128*1024)
//
//#define MAX_MAP_PLANES       32767
//#define MAX_MAP_NODES        32767
//#define MAX_MAP_CLIPNODES    32767
//#define MAX_MAP_LEAFS        8192
//#define MAX_MAP_VERTS        65535
//#define MAX_MAP_FACES        65535
//#define MAX_MAP_MARKSURFACES 65535
//#define MAX_MAP_TEXINFO      8192
//#define MAX_MAP_EDGES        256000
//#define MAX_MAP_SURFEDGES    512000
//#define MAX_MAP_TEXTURES     512
//#define MAX_MAP_MIPTEX       0x200000
//#define MAX_MAP_LIGHTING     0x200000
//#define MAX_MAP_VISIBILITY   0x200000
//
//#define MAX_MAP_PORTALS     65536

/***************************************************************
 * SLumpEntity contains all entity data.
 * Like in map file, entity data is fully copied as ASCII text.
 **************************************************************/
struct SLumpEntity
{
    std::string Data;

    std::vector<SMapEntity> Entities; // memory-only
    std::vector<std::filesystem::path> WadPaths; // memory-only
    std::string SkyBoxPrefix;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * EPlaneType defined plane type.
 * X, Y, Z mean plane is perpendicular to X, Y, Z Axis, i.e. normal is parallel to that Axis.
 * If plane is not perpendicular to any axis, it snapped to the nearest axis.
 * If the angle of one axis and plane normal is the minimum, it's the nearest axis.
 **************************************************************/
enum class EPlaneType
{
    X,
    Y,
    Z,
    ANY_X,
    ANY_Y,
    ANY_Z
};

/***************************************************************
 * SLumpPlane defined a plane, which is represented by Hesse normal form.
 *
 * Hesse normal form: for a plane with normal N, point A is on the plane.
 * For any point P on the plane, (P - A) is perpendicular to N.
 * So (P - A) * N = 0 => P * N = A * N = d, where d is the distance from origin to plane.
 **************************************************************/
struct SBspPlane
{
    SVec3 Normal;
    float DistanceToOrigin;
    int32_t Type; // EPlaneType
};

struct SLumpPlane
{
    std::vector<SBspPlane> Planes;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SBspTexture defined a texture, which can be saved in wad files or bsp itself.
 * Name is needed for texture finding.
 * Offsets can be all zero, meaning this texture should be loaded from wad.
 * Else, Offsets store data offset related to the beginning of SBspTexture.
 * TODO: is width and height set when data is in wad?
 **************************************************************/
struct SBspTexture
{
    std::string Name;
    uint32_t Width, Height;
    uint32_t Offsets[g_BspMipmapLevel];

    bool IsDataInBsp = false; // memory-only
    uint8_t* pData = nullptr; // memory-only

    void read(std::ifstream& vFile, uint64_t vOffset);
    bool getRawRGBAPixels(void* vopData) const;
};

/***************************************************************
 * SLumpTexture defined all texture used in bsp.
 * TexOffsets store begining of SBspTextures related to the beginning of the Lump.
 * And texture pixel data is stored between SBspTextures.
 **************************************************************/
struct SLumpTexture
{
    uint32_t NumTexture;
    std::vector<int32_t> TexOffsets;
    std::vector<SBspTexture> Textures;

    void read(std::ifstream& vFile, uint64_t vOffset);
};

/***************************************************************
 * SLumpVertex store all vertices in bsp.
 **************************************************************/
struct SLumpVertex
{
    std::vector<SVec3> Vertices;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SLumpVisibility store vis data in bsp.
 * Vis can boost up rendering, and it's optional in a bsp.
 * Vis data is a sequences of bitfields which are run-length encoded.
 * run-length encoded: simple compressing method, e.g. AAAABBBBBCCC => 4A5B3C
 * Particularlly in goldsrc, run-length encoding only perform on ZERO, as a lot of zeros in vis data.
 * And other data is simply non-encoded.
 * For example, 00110000 00000000 00000000 000000000 10000000 00000000 00000000
 * -> hex byte form: 0x30 0x00 0x00 0x00 0x00 0x80 0x00 0x00 0x00
 * -> run-length encode: 0x30 (no change) 0x00 (mark for run-length) 0x03 (3 zeros)
 * 0x80 (no change) 0x00 (mark for run-length) 0x02 (2 zeros).
 * Furthermore, leaf 0 contain no data and is outside of any room in map, and vis data start from leaf 1
 **************************************************************/
struct SLumpVisibility
{
    std::vector<uint8_t> Vis;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SBspNode store an internal bsp node.
 * Node data contain the tree structure.
 * PlaneIndex refer to the plane of node (see bsp generation for details).
 * ChildrenIndices is the indices node of children,
 * if > 0, it's node index,
 * if <= 0, it's bitwise inversed value is leave index in leaf lump.
 * Bounding box is AABB (Axis-Aligned Bounding Box).
 * FirstFaceIndex is the start face index in face lump, and NumFace is number of faces.
 **************************************************************/
struct SBspNode
{
    uint32_t PlaneIndex;
    int16_t ChildrenIndices[2];
    int16_t BoundingBoxMin[3];
    int16_t BoundingBoxMax[3];
    uint16_t FirstFaceIndex;
    uint16_t NumFace;
};

/***************************************************************
 * SLumpNode store all internal bsp nodes.
 * Leaves are stored in leaf lump.
 **************************************************************/
struct SLumpNode
{
    std::vector<SBspNode> Nodes;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SBspTexInfo store a texture mapping info.
 * Flags seems always be zero.
 **************************************************************/
struct SBspTexInfo
{
    SVec3 TextureDirectionU;
    float TextureOffsetU;
    SVec3 TextureDirectionV;
    float TextureOffsetV;
    uint32_t TextureIndex;
    uint32_t Flags;

    glm::vec2 getTexCoord(glm::vec3 vVertex) const;
    glm::vec2 getNormalizedTexCoord(glm::vec3 vVertex, size_t vTexWidth, size_t vTexHeight) const;
};

/***************************************************************
 * SLumpTexInfo store all texture mapping infos.
 **************************************************************/
struct SLumpTexInfo
{
    std::vector<SBspTexInfo> TexInfos;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SBspFace store a face.
 * PlaneIndex refers to the plane that is parellel to this face, i.e. parellel normal.
 * nPlaneSide is just a bool, used to specify whether front or back is normal. 
 If 0, same direction, else reverse direction.
 * FirstEdgeIndex and NumEdge refer to edges of this face stored at edge lump.
 * TexInfoIndex referd to used texinfo.
 * LightingStyles? Don't know how to use it.
 * LightmapOffset refers to the lightmap generated by rad.
 **************************************************************/
struct SBspFace
{
    uint16_t PlaneIndex;
    uint16_t PlaneSide;
    uint32_t FirstSurfedgeIndex;
    uint16_t NumSurfedge;
    uint16_t TexInfoIndex;
    uint8_t LightingStyles[4];
    uint32_t LightmapOffset;
};

/***************************************************************
 * SLumpFace store all faces.
 **************************************************************/
struct SLumpFace
{
    std::vector<SBspFace> Faces;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SBspLightmap store a lightmap, which contain 8bit 3 channel color.
 **************************************************************/
struct SBspLightmap
{
    uint8_t R, G, B;
};

/***************************************************************
 * SLumpLighting store all lightmaps.
 * Each face use a lightmap, which is a subset of this array.
 * Lightmap acts like a texture, it's size cover a whole face (size is the same to bounding box).
 * Becareful that in GoldSrc, 16 pixels of texture use 1 luxel in lightmap.
 * And one face can contain limited number of lightmap, and compiler will cut face if a face is too large.
 * You will find more at this link: http://jheriko-rtw.blogspot.com/2010/11/dissecting-quake-2-bsp-format.html.
 * Update: I encountered that faceindex is -1 in uint32_t, seems to represent for no lightmap?
 **************************************************************/
struct SLumpLighting
{
    std::vector<SBspLightmap> Lightmaps;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
    void getRawRGBAPixels(size_t vOffsetInBytes, size_t vSizeInBytes, void* vopData) const;
};

/***************************************************************
 * SBspClipNode store a clipnode.
 * If ChildrenIndices > 0, it's clipnode index,
 * Else it's content type (mentioned in leaf lump).
 **************************************************************/
struct SBspClipNode
{
    int32_t PlaneIndex;
    int16_t ChildrenIndices[2];
};

/***************************************************************
 * SLumpClipNode store all clipnodes.
 * clip tree is another bsp tree used only for collision detection.
 **************************************************************/
struct SLumpClipNode
{
    std::vector<SBspClipNode> ClipNodes;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

enum class EBspContent
{
    EMPTY        = -1,
    SOLID        = -2,
    WATER        = -3,
    SLIME        = -4,
    LAVA         = -5,
    SKY          = -6,
    ORIGIN       = -7,
    CLIP         = -8,
    CURRENT_0    = -9,
    CURRENT_90   = -10,
    CURRENT_180  = -11,
    CURRENT_270  = -12,
    CURRENT_UP   = -13,
    CURRENT_DOWN = -14,
    TRANSLUCENT  = -15
};

/***************************************************************
 * SBspLeaf store a leaf node.
 * VisOffset defines the start of raw PVS data, if -1, no vis data.
 * Bounding box is AABB (Axis-Aligned Bounding Box).
 * AmbientLevels? Don't know how to use it.
 **************************************************************/
struct SBspLeaf
{
    int32_t Content; // EBspContent
    int32_t VisOffset;
    int16_t BoundingBoxMin[3];
    int16_t BoundingBoxMax[3];
    uint16_t FirstMarkSurfaceIndex;
    uint16_t NumMarkSurface;
    uint8_t AmbientLevels[4];
};

/***************************************************************
 * SBspLeaf store ALL leaf nodes.
 **************************************************************/
struct SLumpLeaf
{
    std::vector<SBspLeaf> Leaves;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SLumpMarkSurface simply store a map from marksurface index used by face node to actual face index.
 **************************************************************/
struct SLumpMarkSurface
{
    std::vector<uint16_t> FaceIndices;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SBspEdge refer to an edge between two vertex.
 **************************************************************/
struct SBspEdge
{
    uint16_t VertexIndices[2];
};

/***************************************************************
 * SLumpEdge contains all edges in bsp.
 **************************************************************/
struct SLumpEdge
{
    std::vector<SBspEdge> Edges;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SLumpSurfedge store a map from Surfedge index to actual edge index,
 * and also tell the face the start vertex.
 * I.e. if index >= 0, it point to edge index and face start from 1st vertex to 2nd vertex,
 * otherwise, it point to -index and face start from 2nd vertex to 1st vertex.
 **************************************************************/
struct SLumpSurfedge
{
    std::vector<int32_t> Surfedges;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SBspModel store a model.
 * Use float bounding box. Also AABB (Axis-Aligned Bounding Box).
 * NodeIndices, [0] is the root of mini bsp tree.
 * [1], [2], [3] seems refer to 3 types of collision nodes (stand, big size and crouch).
 * VisLeafs? Don't know what is mean and how to use it.
 * FirstFaceIndex and NumFaces? why use it when there is node index?
 **************************************************************/
struct SBspModel
{
    float BoundingBoxMin[3];
    float BoundingBoxMax[3];
    SVec3 Origin;
    int32_t NodeIndices[g_MaxHull];
    int32_t VisLeafs;
    int32_t FirstFaceIndex;
    int32_t NumFaces;
};

/***************************************************************
 * SLumpModel store all models.
 * A model is like a mini BSP tree.
 * It use local coordinate for vertex related to origin.
 * Seems to be used for brushes belong to entities.
 **************************************************************/
struct SLumpModel
{
    std::vector<SBspModel> Models;

    void read(std::ifstream& vFile, uint64_t vOffset, uint64_t vSize);
};

/***************************************************************
 * SBspLumps Contains all 15 type of lumps in a bsp.
 **************************************************************/
struct SBspLumps
{
    SLumpEntity m_LumpEntity;
    SLumpPlane m_LumpPlane;
    SLumpTexture m_LumpTexture;
    SLumpVertex m_LumpVertex;
    SLumpVisibility m_LumpVisibility;
    SLumpNode m_LumpNode;
    SLumpTexInfo m_LumpTexInfo;
    SLumpFace m_LumpFace;
    SLumpLighting m_LumpLighting;
    SLumpClipNode m_LumpClipNode;
    SLumpLeaf m_LumpLeaf;
    SLumpMarkSurface m_LumpMarkSurface;
    SLumpEdge m_LumpEdge;
    SLumpSurfedge m_LumpSurfedge;
    SLumpModel m_LumpModel;
};

/***************************************************************
 * Class for bsp file reading.
 **************************************************************/
class CIOGoldSrcBsp : public CIOBase 
{
public:
    CIOGoldSrcBsp() : CIOBase() {}
    CIOGoldSrcBsp(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}

    const SBspLumps& getLumps() const { return m_Lumps; }

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    SBspHeader m_Header;
    SBspLumps m_Lumps;
};