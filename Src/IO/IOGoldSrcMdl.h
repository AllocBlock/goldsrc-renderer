#pragma once
#include "IOBase.h"
#include "IOCommon.h"

#include <array>

struct SMdlHeader
{
    char Magic[4];
    int Version;

    char Name[64];
    int Length;

    GoldSrc::SVec3 Eyeposition;    // ideal eye position
    GoldSrc::SVec3 Min;            // ideal movement hull size
    GoldSrc::SVec3 Max;

    GoldSrc::SVec3 AABBMin;            // clipping bounding box
    GoldSrc::SVec3 AABBMax;

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
    int32_t    DataOffset;

    std::vector<uint8_t> IndexSet;
    std::array<GoldSrc::SColor, 256> Palette;

    static size_t getHeaderSize()
    {
        return 64 * sizeof(char) + 4 * sizeof(int32_t);
    }

    void read(std::ifstream& voFile)
    {
        voFile.read(reinterpret_cast<char*>(this), getHeaderSize());
        voFile.seekg(DataOffset, std::ios::beg);
        IndexSet.resize(Width * Height);
        voFile.read(reinterpret_cast<char*>(IndexSet.data()), Width * Height);
        voFile.read(reinterpret_cast<char*>(Palette.data()), 256 * 3 * sizeof(uint8_t));
    }

    void getRawRGBAPixels(void* voData) const
    {
        unsigned char* pIter = static_cast<unsigned char*>(voData);
        for (uint8_t PalatteIndex : IndexSet)
        {
            GoldSrc::SColor PixelColor = Palette[PalatteIndex];
            *pIter++ = static_cast<unsigned char>(PixelColor.R);
            *pIter++ = static_cast<unsigned char>(PixelColor.G);
            *pIter++ = static_cast<unsigned char>(PixelColor.B);
            *pIter++ = static_cast<unsigned char>(0xff);
        }
    }
};

struct SMdlTriangleVertex
{
    int16_t VertexIndex; // index into vertex array
    int16_t NormalIndex; // index into normal array
    int16_t S, T; // s,t position on skin
};

struct SMdlMesh
{
    int32_t TriangleNum = 0;
    int32_t TriangleDataOffset = 0;
    int32_t SkinReference = 0;
    int32_t NormalNum = 0;        // per mesh normals
    int32_t NormalDataOffset = 0; // normal vec3_t

    std::vector<SMdlTriangleVertex> TriangleVertexSet;

    static size_t getHeaderSize()
    {
        return 5 * sizeof(int32_t);
    }
     
    void read(std::ifstream& voFile)
    {
        if (voFile.fail())
            throw std::runtime_error(u8"遇到错误");
        voFile.read(reinterpret_cast<char*>(this), getHeaderSize());

        voFile.seekg(TriangleDataOffset, std::ios::beg);

        int16_t SignedVertexNum = 0;
        do
        {
            voFile.read(reinterpret_cast<char*>(&SignedVertexNum), sizeof(int16_t));
            if (voFile.fail())
                throw std::runtime_error(u8"遇到错误");
            if (SignedVertexNum == 0) break;

            size_t VertexNum = std::abs(SignedVertexNum);
            _ASSERTE(VertexNum >= 3);
            std::vector<SMdlTriangleVertex> TempTriangleVertexSet(VertexNum);
            voFile.read(reinterpret_cast<char*>(TempTriangleVertexSet.data()), VertexNum * sizeof(SMdlTriangleVertex));

            if (SignedVertexNum > 0) // 大于零，代表后面VertexNum个顶点组成一个三角带图元，这里转为三角形图元
            {
                for (size_t i = 0; i < VertexNum - 2; ++i)
                {
                    TriangleVertexSet.emplace_back(TempTriangleVertexSet[i]);
                    if (i % 2 == 0)
                    {
                        TriangleVertexSet.emplace_back(TempTriangleVertexSet[i + 1]);
                        TriangleVertexSet.emplace_back(TempTriangleVertexSet[i + 2]);
                    }
                    else
                    {
                        TriangleVertexSet.emplace_back(TempTriangleVertexSet[i + 2]);
                        TriangleVertexSet.emplace_back(TempTriangleVertexSet[i + 1]);
                    }
                    
                }
            }
            else // 小于零，代表后面-VertexNum个顶点组成一个三角扇图元，这里也转为三角形图元
            {
                for (size_t i = 0; i < VertexNum - 2; ++i)
                {
                    TriangleVertexSet.emplace_back(TempTriangleVertexSet[0]);
                    TriangleVertexSet.emplace_back(TempTriangleVertexSet[i + 1]);
                    TriangleVertexSet.emplace_back(TempTriangleVertexSet[i + 2]);
                }
            }

        } while (true);
    }
};

struct SMdlModel
{
    char Name[64] = "";

    int32_t Type = 0;

    float BoundingRadius = 0.0f;

    int32_t MeshNum = 0;
    int32_t MeshDataOffset = 0;

    int32_t VertexNum = 0;        // number of unique vertices
    int32_t VertexBondIndexDataOffset = 0;    // vertex bone info
    int32_t VertexDataOffset = 0;        // vertex vec3_t
    int32_t NormalNum = 0;        // number of unique surface normals
    int32_t NormalBondIndexDataOffset = 0;    // normal bone info
    int32_t NormalDataOffset = 0;        // normal vec3_t

    int32_t GroupNum = 0;        // deformation groups
    int32_t GroupDataOffset = 0;

    std::vector<SMdlMesh> MeshSet;
    std::vector<GoldSrc::SVec3> VertexSet;
    std::vector<GoldSrc::SVec3> NormalSet;
    std::vector<int16_t> VertexBoneIndexSet;
    std::vector<int16_t> NormalBoneIndexSet;

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
            voFile.seekg(MeshDataOffset + i * MeshHeaderSize, std::ios::beg);
            MeshSet[i].read(voFile);
        }

        VertexSet.resize(VertexNum);
        voFile.seekg(VertexDataOffset, std::ios::beg);
        voFile.read(reinterpret_cast<char*>(VertexSet.data()), VertexNum * sizeof(GoldSrc::SVec3));

        NormalSet.resize(NormalNum);
        voFile.seekg(NormalDataOffset, std::ios::beg);
        voFile.read(reinterpret_cast<char*>(NormalSet.data()), NormalNum * sizeof(GoldSrc::SVec3));

        VertexBoneIndexSet.resize(VertexNum);
        voFile.seekg(VertexBondIndexDataOffset, std::ios::beg);
        voFile.read(reinterpret_cast<char*>(VertexBoneIndexSet.data()), VertexNum * sizeof(int16_t));

        NormalBoneIndexSet.resize(NormalNum);
        voFile.seekg(NormalBondIndexDataOffset, std::ios::beg);
        voFile.read(reinterpret_cast<char*>(NormalBoneIndexSet.data()), NormalNum * sizeof(int16_t));
    }
};

struct SMdlBodyPart
{
    char Name[64] = "";
    int32_t ModelNum = 0;
    int32_t Base = 0;
    int32_t ModelDataOffset = 0;

    std::vector<SMdlModel> ModelSet;

    static size_t getHeaderSize()
    {
        return 64 * sizeof(char) + 3 * sizeof(int32_t);
    }

    void read(std::ifstream& voFile)
    {
        voFile.read(reinterpret_cast<char*>(this), getHeaderSize());

        ModelSet.resize(ModelNum);
        const size_t ModelHeaderSize = SMdlModel::getHeaderSize();
        for (int32_t i = 0; i < ModelNum; ++i)
        {
            voFile.seekg(ModelDataOffset + i * ModelHeaderSize, std::ios::beg);
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

struct SMdlBone
{
    char Name[32]; // bone name for symbolic links
    int ParentIndex = 0; // parent bone
    int FlagBitField = 0; // ??
    int BoneControllerIndexSet[6]; // bone controller index, -1 == none
    float Value[6]; // default DoF values
    float Scale[6]; // scale for delta DoF values
};

struct SMdlBoneController
{
    int Bone = 0;    // -1 == 0
    int Type = 0;    // X, Y, Z, XR, YR, ZR, M
    float Start = 0.0f;
    float End = 0.0f;
    int Rest = 0;    // byte index value at rest
    int Index = 0;    // 0-3 user set controller, 4 mouth
};

struct SMdlSequenceDescription
{
    char				label[32];	// sequence label

    float				fps = 0.0f;		// frames per second	
    int					flags = 0;		// looping/non-looping flags

    int					activity = 0;
    int					actweight = 0;

    int					numevents = 0;
    int					eventindex = 0;

    int					numframes = 0;	// number of frames per sequence

    int					numpivots = 0;	// number of foot pivots
    int					pivotindex = 0;

    int					motiontype = 0;
    int					motionbone = 0;
    GoldSrc::SVec3				linearmovement;
    int					automoveposindex = 0;
    int					automoveangleindex = 0;

    GoldSrc::SVec3				bbmin;		// per sequence bounding box
    GoldSrc::SVec3				bbmax;

    int					numblends = 0;
    int					animindex = 0;		// mstudioanim_t pointer relative to start of sequence group data
                                        // [blend][bone][X, Y, Z, XR, YR, ZR]

    int					blendtype[2];	// X, Y, Z, XR, YR, ZR
    float				blendstart[2];	// starting value
    float				blendend[2];	// ending value
    int					blendparent = 0;

    int					seqgroup = 0;		// sequence group for demand loading

    int					entrynode = 0;		// transition node at entry
    int					exitnode = 0;		// transition node at exit
    int					nodeflags = 0;		// transition rules

    int					nextseq = 0;		// auto advancing sequences
};

// 参考 https://github.com/MoeMod/HLMV-Qt
// 有些许多mdl内的信息并未进行解析
class CIOGoldSrcMdl : public CIOBase
{
public:
    CIOGoldSrcMdl() : CIOBase() {}
    CIOGoldSrcMdl(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}

    const std::vector<SMdlBodyPart>& getBodyParts() const { return m_BodyPartSet; }
    const std::vector<SMdlTexture>& getTextures() const { return m_TextureSet; }
    const std::vector<int16_t>& getSkinReferences() const { return m_SkinReferenceSet; }
protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    SMdlHeader m_Header = {};
    std::vector<SMdlTexture> m_TextureSet;
    std::vector<SMdlBodyPart> m_BodyPartSet;
    std::vector<int16_t> m_SkinReferenceSet;
    std::vector<SMdlBone> m_BoneSet;
    std::vector<SMdlBoneController> m_BoneControllerSet;
    std::vector<SMdlSequenceDescription> m_SequenceDescriptionSet;

    std::vector<glm::mat4> m_BoneTransformSet;
};

