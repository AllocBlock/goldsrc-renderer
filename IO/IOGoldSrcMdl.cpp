#include "IOGoldSrcMdl.h"

#include <vector>
#include <array>

//
//struct SMdlBoundingBox
//{
//    int Bone;
//    int Group;			// intersection group
//    Common::SVec3 min;		// bounding box
//    Common::SVec3 max;
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
//    Common::SVec3 LinearMovement;
//    int AutomovePosindex;
//    int AutomoveAngleindex;
//
//    Common::SVec3 BBMin;		// per sequence bounding box
//    Common::SVec3 BBMax;
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

    // 读取皮肤索引（Skin Reference，Mesh中存储了SkinRef字段，对应了m_SkinReferenceSet数组的索引，而m_SkinReferenceSet存储了对应纹理的索引
    m_SkinReferenceSet.resize(m_Header.SkinReferenceNum);
    File.seekg(m_Header.SkinDataOffset, std::ios::beg);
    File.read(reinterpret_cast<char*>(m_SkinReferenceSet.data()), m_Header.SkinReferenceNum * sizeof(int16_t));

    // 读取骨骼
    m_BoneSet.resize(m_Header.BoneNum);
    File.seekg(m_Header.BoneDataOffset, std::ios::beg);
    File.read(reinterpret_cast<char*>(m_BoneSet.data()), m_Header.BoneNum * sizeof(SMdlBone));

    m_BoneControllerSet.resize(m_Header.BoneControllerNum);
    File.seekg(m_Header.BoneControllerDataOffset, std::ios::beg);
    File.read(reinterpret_cast<char*>(m_BoneControllerSet.data()), m_Header.BoneControllerNum * sizeof(SMdlBoneController));

    // 读取序列
    m_SequenceDescriptionSet.resize(m_Header.SequenceNum);
    File.seekg(m_Header.SequenceDataOffset, std::ios::beg);
    File.read(reinterpret_cast<char*>(m_SequenceDescriptionSet.data()), m_Header.SequenceNum * sizeof(SMdlSequenceDescription));

    // 计算骨骼变换
    m_BoneTransformSet.resize(m_Header.BoneNum);
    // TODO: 太复杂了，先缓一缓...

    // 更新骨骼级联关系
    std::vector<bool> BoneUpdateMarkSet(m_Header.BoneNum, false);
    size_t UpdatedBoneNum = 0;
    size_t Index = 0;
    while (UpdatedBoneNum < m_Header.BoneNum)
    {
        if (BoneUpdateMarkSet[Index]) continue;
        else if (m_BoneSet[Index].ParentIndex == -1) // 无父节点，无需更新
        {
            BoneUpdateMarkSet[Index] = true;
            ++UpdatedBoneNum;
            continue;
        }
        else if (BoneUpdateMarkSet[m_BoneSet[Index].ParentIndex]) // 父节点已更新，则可以更新自己
        {
            m_BoneTransformSet[Index] = m_BoneTransformSet[m_BoneSet[Index].ParentIndex] * m_BoneTransformSet[Index];
            BoneUpdateMarkSet[Index] = true;
            ++UpdatedBoneNum;
            continue;
        }
    }

    return true;
}