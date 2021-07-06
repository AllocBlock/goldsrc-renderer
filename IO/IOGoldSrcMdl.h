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
};

