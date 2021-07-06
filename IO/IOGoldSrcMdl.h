#pragma once
#include "IOBase.h"
#include "IOCommon.h"

struct SMdlHeader
{
    char Magic[4];
    int Version;

    char Name[64];
    int Length;

    IOCommon::SGoldSrcVec3 Eyeposition;    // ideal eye position
    IOCommon::SGoldSrcVec3 Min;            // ideal movement hull size
    IOCommon::SGoldSrcVec3 Max;

    IOCommon::SGoldSrcVec3 AABBMin;            // clipping bounding box
    IOCommon::SGoldSrcVec3 AABBMax;

    int                    FlagBitField;

    int                    BoneNum;            // bones
    int                    BoneDataOffset;

    int                    BoneControllerNum;        // bone controllers
    int                    BoneControllerDataOffset;

    int                    HitboxNum;            // complex bounding boxes
    int                    HitboxDataOffset;

    int                    SequenceNum;                // animation sequences
    int                    SequenceDataOffset;

    int                    SequenceGroupNum;        // demand loaded sequences
    int                    SequenceGroupDataOffset;

    int                    TextureNum;        // raw textures
    int                    TextureDataOffset;
    int                    TextureDataDataOffset;

    int                    SkinReferenceNum;            // replaceable textures
    int                    SkinFamilyNum;
    int                    SkinDataOffset;

    int                    BodyPartNum;
    int                    BodyPartDataOffset;

    int                    AttachmentNum;        // queryable attachable points
    int                    AttachmentDataOffset;

    int                    SoundTable;
    int                    SoundIndex;
    int                    SoundGroupNum;
    int                    SoundGroupDataOffset;

    int                    TransitionNum;        // animation node to animation node transition graph
    int                    TransitionDataOffset;
};


// 存储mdl文件内的纹理
// 从纹理块起始，若有n张纹理，则先是n个SMdlTexture数据
// 紧跟着是图片数据，其起始索引通过SMdlTexture内的DataOffset得到，先存储了Width*Height个8位索引，紧跟256色24位调色板
struct SMdlTexture
{
    char Name[64];
    int32_t FlagBitField;
    int32_t Width;
    int32_t Height;
    int32_t	DataOffset;

    std::vector<uint8_t> IndexSet;
    std::array<IOCommon::SGoldSrcColor, 256> Palette;

    static size_t getHeaderSize()
    {
        return 64 * sizeof(char) + 4 * sizeof(int32_t);
    }

    void read(std::ifstream& voFile)
    {
        voFile.read(reinterpret_cast<char*>(this), getHeaderSize());
        voFile.seekg(DataOffset);
        IndexSet.resize(Width * Height);
        voFile.read(reinterpret_cast<char*>(IndexSet.data()), Width * Height);
        voFile.read(reinterpret_cast<char*>(Palette.data()), 256 * 3 * sizeof(float));
    }
};

struct SMdlMesh
{
    int32_t TriangleNum;
    int32_t TriangleDataOffset;
    int32_t SkinReference;
    int32_t NormalNum;		// per mesh normals
    int32_t NormalDataOffset; // normal vec3_t

    std::vector<

    static size_t getHeaderSize()
    {
        return 5 * sizeof(int32_t);
    }
     
    void read(std::ifstream& voFile)
    {
        voFile.read(reinterpret_cast<char*>(this), getHeaderSize());

        MeshSet.resize(MeshNum);
        const size_t MeshHeaderSize = SMdlMesh::getHeaderSize();
        for (int32_t i = 0; i < MeshNum; ++i)
        {
            voFile.seekg(MeshDataOffset + i * MeshHeaderSize);
            MeshSet[i].read(voFile);
        }
    }
};

struct SMdlModel
{
    char Name[64];

    int32_t Type;

    float BoundingRadius;

    int32_t MeshNum;
    int32_t MeshDataOffset;

    int32_t VertexNum;		// number of unique vertices
    int32_t VertexInfoDataOffset;	// vertex bone info
    int32_t VertexDataOffset;		// vertex vec3_t
    int32_t NormalNum;		// number of unique surface normals
    int32_t NormalInfoDataOffset;	// normal bone info
    int32_t NormalDataOffset;		// normal vec3_t

    int32_t GroupNum;		// deformation groups
    int32_t GroupDataOffset;

    std::vector<SMdlMesh> MeshSet;

    static size_t getHeaderSize()
    {
        return 64 * sizeof(char) + sizeof(float) + 11 * sizeof(int32_t);
    }

    void read(std::ifstream& voFile)
    {
        voFile.read(reinterpret_cast<char*>(this), getHeaderSize());

        MeshSet.resize(MeshNum);
        const size_t MeshHeaderSize = SMdlMesh::getHeaderSize();
        for (int32_t i = 0; i < MeshNum; ++i)
        {
            voFile.seekg(MeshDataOffset + i * MeshHeaderSize);
            MeshSet[i].read(voFile);
        }
    }
};

struct SMdlBodyPart
{
    char Name[64];
    int32_t ModelNum;
    int32_t Base;
    int32_t ModelDataOffset;

    std::vector<SMdlModel> ModelSet;

    static size_t getHeaderSize()
    {
        return sizeof(char) + 3 * sizeof(int32_t);
    }

    void read(std::ifstream& voFile)
    {
        voFile.read(reinterpret_cast<char*>(this), getHeaderSize());

        ModelSet.resize(ModelNum);
        const size_t ModelHeaderSize = SMdlModel::getHeaderSize();
        for (int32_t i = 0; i < ModelNum; ++i)
        {
            voFile.seekg(ModelDataOffset + i * ModelHeaderSize);
            ModelSet[i].read(voFile);
        }
    }
};

struct SMdlSkin
{
    std::vector<int16_t> RefToTextureIndex;

    void read(std::ifstream& voFile, int vSkinNum)
    {
        RefToTextureIndex.resize(vSkinNum);
        voFile.read(reinterpret_cast<char*>(RefToTextureIndex.data()), vSkinNum * sizeof(int16_t));
    }
};

// 参考 https://github.com/MoeMod/HLMV-Qt
// 有些许多mdl内的信息并未进行解析
class CIOGoldSrcMdl : public CIOBase
{
public:
    CIOGoldSrcMdl() : CIOBase() {}
    CIOGoldSrcMdl(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}
protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    SMdlHeader m_Header = {};
    std::vector<SMdlTexture> m_TextureSet;
    std::vector<SMdlBodyPart> m_BodyPartSet;
};

