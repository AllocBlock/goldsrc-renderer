#pragma once

#include "IOCommon.h"
#include "IOBase.h"
#include "Pointer.h"
#include <vector>

struct SRmfVariableString
{
    uint8_t Length;
    std::string String;

    void read(std::ifstream& vFile);
};

struct SRmfEntityProperty
{
    SRmfVariableString Key; // 32 bytes max
    SRmfVariableString Value; // 100 bytes max

    void read(std::ifstream& vFile);
};

/***************************************************************
 * SRmfVisGroup stores visibility group (not sure if this spells corrently).
 * VisGroup is used to select/show/hide grouped objects for more convinient map editing.
 **************************************************************/
struct SRmfVisGroup
{
    char Name[128]; // Name of the VisGroup
    Common::GoldSrc::SColor Color; // Border color in VHE of this group
    char Unknown;
    int Index; // Index of this group. Used by objects.
    uint8_t IsVisible; // If this group visible. 1 = visible, 0 = not visible.
    char Unknown2[3];

    void read(std::ifstream& vFile);
};

/***************************************************************
 * SRmfFace stores a face and its texture info.
 **************************************************************/
struct SRmfFace
{
    char TextureName[256]; // Name of texture
    float Unknown;
    Common::GoldSrc::SVec3 TextureDirectionU;
    float TextureOffsetU;
    Common::GoldSrc::SVec3 TextureDirectionV;
    float TextureOffsetV;
    float TextureRotation; // In degrees
    float TextureScaleU;
    float TextureScaleV;
    char Unknown2[16];
    int  VertexNum; // Number of vertices
    std::vector<Common::GoldSrc::SVec3> Vertices; // Vertices data in clockwise
    Common::GoldSrc::SVec3 PlanePoints[3]; // Used for defining plane of this face. VHE directly use first 3 vertices rather than this;

    void read(std::ifstream& vFile);
};

/***************************************************************
 * SRmfObject is base struct of solid, entity and group in rmf.
 **************************************************************/
struct SRmfObject
{
    SRmfVariableString Type; // type of this object. Could be "CMapSolid", "CMapEntity" or "CMapGroup".
    int VisGroupIndex; // Each Object belongs to one and only-one VisGroup.
    
    Common::GoldSrc::SColor Color; // The display color in VHE.

    static ptr<SRmfObject> create(std::ifstream& vFile);
protected:
    virtual void _readV(std::ifstream& vFile) = 0;
private:
    void __read(std::ifstream& vFile);
};

/***************************************************************
 * SRmfSolid is a solid object.
 * Type = "CMapSolid"
 * Color is used only when it's not in any Entity or VisGroup.
 **************************************************************/
struct SRmfSolid : SRmfObject
{
    char Unknown[4];
    int FaceNum;
    std::vector<SRmfFace> Faces;

protected:
    virtual void _readV(std::ifstream& vFile) override;
};

/***************************************************************
 * SRmfEntity is a entity object.
 * Type = "CMapEntity"
 * VHE draw all Entity as "Purple" color regardless of value of Color.
 **************************************************************/
struct SRmfEntity : SRmfObject
{
    int SolidNum; // 0 = Point entity
    std::vector<ptr<SRmfSolid>> Solids;
    SRmfVariableString ClassName; // 128 byte max
    char Unknown[4];
    int EntityFlags; // Bit field entity flags (refers to the checkboxes in properties dialog)
    int PropertyNum; // Number of properties (key-value pairs)
    std::vector<SRmfEntityProperty> Properties;
    char Unknown2[14];
    Common::GoldSrc::SVec3 Origin; // Position of entity in world coordinates (only used for point entities)
    char Unknown3[4];

protected:
    virtual void _readV(std::ifstream& vFile) override;
};

/***************************************************************
 * SRmfGroup is a entity object.
 * Type = "CMapGroup"
 * Color will override its inside objects.
 **************************************************************/
struct SRmfGroup : SRmfObject
{
    int ObjectNum; // Number of objects in this group (An object is a solid, entity or group)
    std::vector<ptr<SRmfObject>> Objects;

protected:
    virtual void _readV(std::ifstream& vFile) override;
};

/***************************************************************
 * SRmfWorld contains all other objects.
 * Type = "CMapWorld"
 * VisGroup and Color are not used.
 **************************************************************/
struct SRmfWorld : SRmfObject
{
    int ObjectNum;
    std::vector<ptr<SRmfObject>> Objects;
    SRmfVariableString ClassName; // Should be "worldspawn"
    char Unknown[4];
    int EntityFlags; // Probably unused
    int PropertyNum; // Number of properties (key-value pairs)
    std::vector<SRmfEntityProperty> Properties; // Standard keys are "classname" = "worldspawn", "sounds" = "#", "MaxRange" = "#", "mapversion" = "220"
    char Unknown2[12];

protected:
    virtual void _readV(std::ifstream& vFile) override;
};

/***************************************************************
 * SRmfCorner is a path point in a path.
 * PropertyNum and Properties are supposed to be entity properties for the corner but VHE doesn't save them correctly.
 **************************************************************/
struct SRmfCorner
{
    Common::GoldSrc::SVec3 Origin; // Position in world coordinates
    int Index; // Index is used for generating targetnames (corner01, corner02, etc..).
    char Name[128];
    int PropertyNum;
    std::vector<SRmfEntityProperty> Properties;

    void read(std::ifstream& vFile);
};

/***************************************************************
 * SRmfPath is a path generated by path tool in VHE.
 **************************************************************/
struct SRmfPath
{
    char Name[128];
    char ClassName[128]; // should be "path_corner" or "path_track"
    int PathType; // 0 = one way, 1 = circular, 2 = ping-pong
    int CornerNum;
    std::vector<SRmfCorner> Corners;

    void read(std::ifstream& vFile);
};

/***************************************************************
 * SRmfCamera is a camera generated in VHE.
 **************************************************************/
struct SRmfCamera
{
    Common::GoldSrc::SVec3 EyePosition; // Eye position of the camera
    Common::GoldSrc::SVec3 Direction; // Direction of the camera
};

/***************************************************************
 * Class for rmf file reading.
 * BTW, rmf file doesn't contain wads data (wads are setted in VHE setting).
 * m_Wads is prepared for future user selection.
 **************************************************************/
class CIOGoldSrcRmf : public CIOBase 
{
public:
    CIOGoldSrcRmf() : CIOBase() {}
    CIOGoldSrcRmf(std::filesystem::path vFilePath) : CIOBase(vFilePath) {}

    float getVersion() const;
    const std::vector<SRmfVisGroup>& getVisGroups() const;
    const ptr<SRmfWorld>& getWorld() const;
    const std::vector<SRmfPath>& getPaths() const;
    size_t getActiveCameraIndex() const;
    const std::vector<SRmfCamera>& getCameras() const;

protected:
    virtual bool _readV(std::filesystem::path vFilePath) override;

private:
    float Version = 0.0f; // ¡Ö 2.2, CD CC 0C 40 in hex
    char Magic[3] = { 0 }; // Header, always "RMF\0"
    int VisGroupNum = 0;
    std::vector<SRmfVisGroup> VisGroups;
    ptr<SRmfWorld> pWorld;
    int PathNum = 0;
    std::vector<SRmfPath> Paths;
    char Mark[8] = { 0 }; // Should be "DOCINFO"
    float CameraDataVersion = 0.0f; // Probably camera data version
    int ActiveCameraIndex = 0;
    int CameraNum = 0;
    std::vector<SRmfCamera> Cameras;
};