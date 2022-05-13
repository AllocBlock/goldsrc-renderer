#include "IOCommon.h"
#include "IOGoldSrcRmf.h"

#include <fstream>

using namespace Common;

float CIOGoldSrcRmf::getVersion() const
{
    return Version;
}

const std::vector<SRmfVisGroup>& CIOGoldSrcRmf::getVisGroups() const
{
    return VisGroups;
}

const ptr<SRmfWorld>& CIOGoldSrcRmf::getWorld() const
{
    return pWorld;
}

const std::vector<SRmfPath>& CIOGoldSrcRmf::getPaths() const
{
    return Paths;
}

size_t CIOGoldSrcRmf::getActiveCameraIndex() const
{
    return ActiveCameraIndex;
}

const std::vector<SRmfCamera>& CIOGoldSrcRmf::getCameras() const
{
    return Cameras;
}


bool CIOGoldSrcRmf::_readV(std::filesystem::path vFilePath)
{
    std::ifstream File;
    File.open(vFilePath.string(), std::ios::in | std::ios::binary);
    if (!File.is_open())
    {
        Log::log(u8"打开文件 [" + vFilePath.u8string() + u8"] 失败，无权限或文件不存在");
        return false;
    }
    File.read(reinterpret_cast<char*>(&Version), sizeof(float));
    File.read(Magic, sizeof(Magic));
    File.read(reinterpret_cast<char*>(&VisGroupNum), sizeof(int));
    VisGroups.resize(VisGroupNum);
    for (SRmfVisGroup& VisGroup : VisGroups)
        VisGroup.read(File);
    pWorld = std::reinterpret_pointer_cast<SRmfWorld>(SRmfObject::create(File));

    File.read(reinterpret_cast<char*>(&PathNum), sizeof(int));
    Paths.resize(PathNum);
    for (SRmfPath& Path : Paths)
        Path.read(File);

    File.read(Mark, sizeof(Mark));
    if (std::string(Mark) != "DOCINFO")
        throw std::runtime_error(u8"rmf文件中标记位置应该为DOCINFO，而实际为：" + std::string(Mark));
    
    File.read(reinterpret_cast<char*>(&CameraDataVersion), sizeof(float));
    File.read(reinterpret_cast<char*>(&ActiveCameraIndex), sizeof(int));
    File.read(reinterpret_cast<char*>(&CameraNum), sizeof(int));
    Cameras = IO::readArray<SRmfCamera>(File, sizeof(SRmfCamera) * CameraNum);

    return true;
}

void SRmfVariableString::read(std::ifstream& vFile)
{
    vFile.read(reinterpret_cast<char*>(&Length), sizeof(uint8_t));
    String.resize(Length);
    vFile.read(String.data(), sizeof(char) * Length);
    String.pop_back(); // Remove the last \0
}

void SRmfEntityProperty::read(std::ifstream& vFile)
{
    Key.read(vFile);
    Value.read(vFile);
}

void SRmfFace::read(std::ifstream& vFile)
{
    vFile.read(TextureName, sizeof(TextureName));
    vFile.read(reinterpret_cast<char*>(&Unknown), sizeof(float));
    vFile.read(reinterpret_cast<char*>(&TextureDirectionU), sizeof(GoldSrc::SVec3));
    vFile.read(reinterpret_cast<char*>(&TextureOffsetU), sizeof(float));
    vFile.read(reinterpret_cast<char*>(&TextureDirectionV), sizeof(GoldSrc::SVec3));
    vFile.read(reinterpret_cast<char*>(&TextureOffsetV), sizeof(float));
    vFile.read(reinterpret_cast<char*>(&TextureRotation), sizeof(float));
    vFile.read(reinterpret_cast<char*>(&TextureScaleU), sizeof(float));
    vFile.read(reinterpret_cast<char*>(&TextureScaleV), sizeof(float));
    vFile.read(Unknown2, sizeof(Unknown2));
    vFile.read(reinterpret_cast<char*>(&VertexNum), sizeof(int));
    Vertices = IO::readArray<GoldSrc::SVec3>(vFile, sizeof(GoldSrc::SVec3) * VertexNum);
    vFile.read(reinterpret_cast<char*>(&PlanePoints), sizeof(PlanePoints));
}

void SRmfVisGroup::read(std::ifstream& vFile)
{
    vFile.read(Name, sizeof(Name));
    vFile.read(reinterpret_cast<char*>(&Color), sizeof(GoldSrc::SColor));
    vFile.read(reinterpret_cast<char*>(&Unknown), sizeof(char));
    vFile.read(reinterpret_cast<char*>(&Index), sizeof(int));
    vFile.read(reinterpret_cast<char*>(&IsVisible), sizeof(uint8_t));
    vFile.read(Unknown2, sizeof(Unknown2));
}

void SRmfObject::__read(std::ifstream& vFile)
{
    Type.read(vFile);
    vFile.read(reinterpret_cast<char*>(&VisGroupIndex), sizeof(int));
    vFile.read(reinterpret_cast<char*>(&Color), sizeof(GoldSrc::SColor));
    _readV(vFile);
}

void SRmfWorld::_readV(std::ifstream& vFile)
{
    vFile.read(reinterpret_cast<char*>(&ObjectNum), sizeof(int));
    Objects.resize(ObjectNum);
    for(ptr<SRmfObject>& pObject : Objects)
        pObject = SRmfObject::create(vFile);
    ClassName.read(vFile);
    if (ClassName.String != "worldspawn")
        throw std::runtime_error(u8"世界实体(worldspawn)类型错误：" + ClassName.String);
    vFile.read(Unknown, sizeof(Unknown));
    vFile.read(reinterpret_cast<char*>(&EntityFlags), sizeof(int));
    vFile.read(reinterpret_cast<char*>(&PropertyNum), sizeof(PropertyNum));
    Properties.resize(PropertyNum);
    for (SRmfEntityProperty& Property : Properties)
        Property.read(vFile);
    vFile.read(Unknown2, sizeof(Unknown2));
}

void SRmfSolid::_readV(std::ifstream& vFile)
{
    vFile.read(Unknown, sizeof(Unknown));
    vFile.read(reinterpret_cast<char*>(&FaceNum), sizeof(int));
    Faces.resize(FaceNum);
    for (SRmfFace& Face : Faces)
        Face.read(vFile);
}

void SRmfEntity::_readV(std::ifstream& vFile)
{
    vFile.read(reinterpret_cast<char*>(&SolidNum), sizeof(int));
    Solids.resize(SolidNum);
    for (ptr<SRmfSolid>& pSolid : Solids)
        pSolid = std::reinterpret_pointer_cast<SRmfSolid>(SRmfObject::create(vFile));
    ClassName.read(vFile);
    vFile.read(Unknown, sizeof(Unknown));
    vFile.read(reinterpret_cast<char*>(&EntityFlags), sizeof(int));
    vFile.read(reinterpret_cast<char*>(&PropertyNum), sizeof(int));
    Properties.resize(PropertyNum);
    for (SRmfEntityProperty& Property : Properties)
        Property.read(vFile);
    vFile.read(Unknown2, sizeof(Unknown2));
    vFile.read(reinterpret_cast<char*>(&Origin), sizeof(GoldSrc::SVec3));
    vFile.read(Unknown3, sizeof(Unknown3));
}

void SRmfGroup::_readV(std::ifstream& vFile)
{
    vFile.read(reinterpret_cast<char*>(&ObjectNum), sizeof(int));
    Objects.resize(ObjectNum);
    for (ptr<SRmfObject>& pObject : Objects)
        pObject = SRmfObject::create(vFile);
}

ptr<SRmfObject> SRmfObject::create(std::ifstream& vFile)
{
    std::streampos OriginPosition = vFile.tellg();
    SRmfVariableString ObjectHeader;
    ObjectHeader.read(vFile);
    vFile.seekg(OriginPosition);
    if (ObjectHeader.String == "CMapWorld")
    {
        auto pWorld = make<SRmfWorld>();
        pWorld->__read(vFile);
        return pWorld;
    }
    else if (ObjectHeader.String == "CMapSolid")
    {
        auto pSolid = make<SRmfSolid>();
        pSolid->__read(vFile);
        return pSolid;
    }
    else if (ObjectHeader.String == "CMapEntity")
    {
        auto pEntity = make<SRmfEntity>();
        pEntity->__read(vFile);
        return pEntity;
    }
    else if (ObjectHeader.String == "CMapGroup")
    {
        auto pGroup = make<SRmfGroup>();
        pGroup->__read(vFile);
        return pGroup;
    }
    else
    {
        throw std::runtime_error(u8"rmf对象类型错误：" + ObjectHeader.String);
    }
}

void SRmfCorner::read(std::ifstream& vFile)
{
    vFile.read(reinterpret_cast<char*>(&Origin), sizeof(GoldSrc::SVec3));
    vFile.read(Name, sizeof(Name));
    vFile.read(reinterpret_cast<char*>(&PropertyNum), sizeof(int));

    Properties.resize(PropertyNum);
    for (SRmfEntityProperty& Property : Properties)
        Property.read(vFile);
}

void SRmfPath::read(std::ifstream& vFile)
{
    vFile.read(Name, sizeof(Name));
    vFile.read(ClassName, sizeof(ClassName));
    if (ClassName != "path_corner" && ClassName != "path_track")
        throw std::runtime_error(u8"路径点类型错误，应为path_corner或path_track，而实际为：" + std::string(ClassName));
    vFile.read(reinterpret_cast<char*>(&PathType), sizeof(int));
    vFile.read(reinterpret_cast<char*>(&CornerNum), sizeof(int));

    Corners.resize(CornerNum);
    for (SRmfCorner& Corner : Corners)
        Corner.read(vFile);
}
