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
    int32_t    DataOffset;

    std::vector<uint8_t> IndexSet;
    std::array<IOCommon::SGoldSrcColor, 256> Palette;

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
            IOCommon::SGoldSrcColor PixelColor = Palette[PalatteIndex];
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
    int32_t TriangleNum;
    int32_t TriangleDataOffset;
    int32_t SkinReference;
    int32_t NormalNum;        // per mesh normals
    int32_t NormalDataOffset; // normal vec3_t

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
    char Name[64];

    int32_t Type;

    float BoundingRadius;

    int32_t MeshNum;
    int32_t MeshDataOffset;

    int32_t VertexNum;        // number of unique vertices
    int32_t VertexBondIndexDataOffset;    // vertex bone info
    int32_t VertexDataOffset;        // vertex vec3_t
    int32_t NormalNum;        // number of unique surface normals
    int32_t NormalBondIndexDataOffset;    // normal bone info
    int32_t NormalDataOffset;        // normal vec3_t

    int32_t GroupNum;        // deformation groups
    int32_t GroupDataOffset;

    std::vector<SMdlMesh> MeshSet;
    std::vector<IOCommon::SGoldSrcVec3> VertexSet;
    std::vector<IOCommon::SGoldSrcVec3> NormalSet;
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
        voFile.read(reinterpret_cast<char*>(VertexSet.data()), VertexNum * sizeof(IOCommon::SGoldSrcVec3));

        NormalSet.resize(NormalNum);
        voFile.seekg(NormalDataOffset, std::ios::beg);
        voFile.read(reinterpret_cast<char*>(NormalSet.data()), NormalNum * sizeof(IOCommon::SGoldSrcVec3));

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
    char Name[64];
    int32_t ModelNum;
    int32_t Base;
    int32_t ModelDataOffset;

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
    int ParentIndex; // parent bone
    int FlagBitField; // ??
    int BoneControllerIndexSet[6]; // bone controller index, -1 == none
    float Value[6]; // default DoF values
    float Scale[6]; // scale for delta DoF values
};

struct SMdlBoneController
{
    int Bone;    // -1 == 0
    int Type;    // X, Y, Z, XR, YR, ZR, M
    float Start;
    float End;
    int Rest;    // byte index value at rest
    int Index;    // 0-3 user set controller, 4 mouth
};

struct SMdlSequenceDescription
{
    char				label[32];	// sequence label

    float				fps;		// frames per second	
    int					flags;		// looping/non-looping flags

    int					activity;
    int					actweight;

    int					numevents;
    int					eventindex;

    int					numframes;	// number of frames per sequence

    int					numpivots;	// number of foot pivots
    int					pivotindex;

    int					motiontype;
    int					motionbone;
    IOCommon::SGoldSrcVec3				linearmovement;
    int					automoveposindex;
    int					automoveangleindex;

    IOCommon::SGoldSrcVec3				bbmin;		// per sequence bounding box
    IOCommon::SGoldSrcVec3				bbmax;

    int					numblends;
    int					animindex;		// mstudioanim_t pointer relative to start of sequence group data
                                        // [blend][bone][X, Y, Z, XR, YR, ZR]

    int					blendtype[2];	// X, Y, Z, XR, YR, ZR
    float				blendstart[2];	// starting value
    float				blendend[2];	// ending value
    int					blendparent;

    int					seqgroup;		// sequence group for demand loading

    int					entrynode;		// transition node at entry
    int					exitnode;		// transition node at exit
    int					nodeflags;		// transition rules

    int					nextseq;		// auto advancing sequences
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

