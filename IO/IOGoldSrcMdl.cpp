#include "IOGoldSrcMdl.h"

#include <vector>
#include <array>

//struct SMdlBone
//{
//    char Name[32];	// bone name for symbolic links
//    int ParentIndex;		// parent bone
//    int FlagBitField;		// ??
//    int BoneControllerIndexSet[6];	// bone controller index, -1 == none
//    float Value[6];	// default DoF values
//    float Scale[6]; // scale for delta DoF values
//};
//
//struct SMdlBoneController
//{
//    int Bone;	// -1 == 0
//    int Type;	// X, Y, Z, XR, YR, ZR, M
//    float Start;
//    float End;
//    int Rest;	// byte index value at rest
//    int Index;	// 0-3 user set controller, 4 mouth
//};
//
//struct SMdlBoundingBox
//{
//    int Bone;
//    int Group;			// intersection group
//    IOCommon::SGoldSrcVec3 min;		// bounding box
//    IOCommon::SGoldSrcVec3 max;
//};
//
//struct SMdlSequenceGroup
//{
//    char Label[32];	// textual name
//    char Name[64]; // file name
//    int Cache; // cache index pointer
//    int Data; // hack for group 0
//};
//
//struct SMdlSequenceDescription
//{
//    char Label[32];	// sequence label
//
//    float FPS;		// frames per second	
//    int FlagBitField;		// looping/non-looping flags
//
//    int Activity;
//    int ActWeight;
//
//    int EventNum;
//    int EventDataOffset;
//
//    int FrameNum;	// number of frames per sequence
//
//    int PivotNum;	// number of foot pivots
//    int PivotDataOffset;
//
//    int MotionType;
//    int MotionBone;
//    IOCommon::SGoldSrcVec3 LinearMovement;
//    int AutomovePosindex;
//    int AutomoveAngleindex;
//
//    IOCommon::SGoldSrcVec3 BBMin;		// per sequence bounding box
//    IOCommon::SGoldSrcVec3 BBMax;
//
//    int BlendNum;
//    int AnimationDataOffset;		// mstudioanim_t pointer relative to start of sequence group data
//                                        // [blend][bone][X, Y, Z, XR, YR, ZR]
//
//    int BlendType[2];	// X, Y, Z, XR, YR, ZR
//    float BlendStart[2];	// starting value
//    float BlendEnd[2];	// ending value
//    int BlendParent;
//
//    int SequenceGroup;		// sequence group for demand loading
//
//    int EntryNode;		// transition node at entry
//    int ExitNode;		// transition node at exit
//    int NodeFlags;		// transition rules
//
//    int NextSeq;		// auto advancing sequences
//};

bool CIOGoldSrcMdl::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File(vFilePath, std::ios::in | std::ios::binary);
    if (!File.is_open())
        throw std::runtime_error(u8"文件读取失败");

    File.read(reinterpret_cast<char*>(&m_Header), sizeof(SMdlHeader));
    _ASSERTE(m_Header.Magic[0] == 'I' &&
        m_Header.Magic[1] == 'D' &&
        m_Header.Magic[2] == 'S' &&
        (m_Header.Magic[3] == 'T' || m_Header.Magic[3] == 'Q'));

    // 读取纹理
    m_TextureSet.resize(m_Header.TextureNum);
    const size_t TextureHeaderSize = SMdlTexture::getHeaderSize();
    for (int i = 0; i < m_Header.TextureNum; ++i)
    {
        File.seekg(m_Header.TextureDataOffset + i * TextureHeaderSize, std::ios::beg);
        m_TextureSet[i].read(File);
    }
    
    // 读取部件
    m_BodyPartSet.resize(m_Header.BodyPartNum);
    const size_t BodyPartHeaderSize = SMdlBodyPart::getHeaderSize();
    for (int i = 0; i < m_Header.BodyPartNum; ++i)
    {
        File.seekg(m_Header.BodyPartDataOffset + i * BodyPartHeaderSize, std::ios::beg);
        m_BodyPartSet[i].read(File);
    }
}