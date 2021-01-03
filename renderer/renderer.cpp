#include "Interactor.h"
#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <set>
#include "IOGoldSrcMap.h"
#include "IOGoldSrcWad.h"
#include "IOImage.h"

void readObj(std::string vFileName, CVulkanRenderer& voRenderer);
void readMap(std::string vFileName, CVulkanRenderer& voRenderer);
bool findFile(std::string vFilePath, std::string vSearchDir, std::string& voFilePath);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* pWindow = glfwCreateWindow(800, 600, "Vulkan Simple Render", nullptr, nullptr);
	CVulkanRenderer Renderer(pWindow);
    //readMap("../data/test.map", Renderer);
    readObj("../data/ball.obj", Renderer);
	Renderer.getCamera()->setPos(glm::vec3(0.0f, 0.0f, 3.0f));
    Renderer.init(); 

	CInteractor Interactor(&Renderer);
	Interactor.bindEvent();

	std::chrono::milliseconds LastTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
	while (!glfwWindowShouldClose(pWindow))
	{
		glfwPollEvents();
		std::chrono::milliseconds CurrentTimeStamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch());
		float DeltaTime = static_cast<float>((CurrentTimeStamp - LastTimeStamp).count()) / 1000.0f;
		Interactor.update(DeltaTime);
		Renderer.render();
		LastTimeStamp = CurrentTimeStamp;
	}
	Renderer.waitDevice();
	glfwDestroyWindow(pWindow);
	glfwTerminate();
	return 0;
}

CIOImage generateBlackPurpleGrid(size_t vNumRow, size_t vNumCol, size_t vCellSize)
{
    unsigned char BaseColor1[3] = { 0, 0, 0 };
    unsigned char BaseColor2[3] = { 255, 0, 255 };
    size_t DataSize = vNumRow * vNumCol * vCellSize * vCellSize * 4;
    unsigned char* pData = new unsigned char[DataSize];
    for (size_t i = 0; i < DataSize / 4; i++)
    {
        size_t GridRowIndex = (i / (vNumCol * vCellSize)) / vCellSize;
        size_t GridColIndex = (i % (vNumCol * vCellSize)) / vCellSize;

        unsigned char* pColor;
        if ((GridRowIndex + GridColIndex) % 2 == 0)
            pColor = BaseColor1;
        else
            pColor = BaseColor2;
        pData[i * 4] = pColor[0];
        pData[i * 4 + 1] = pColor[1];
        pData[i * 4 + 2] = pColor[2];
        pData[i * 4 + 3] = static_cast<unsigned char>(255);
    }

    // cout 
    /*for (size_t i = 0; i < vNumRow * vCellSize; i++)
    {
        for (size_t k = 0; k < vNumCol * vCellSize; k++)
        {
            std::cout << pData[(i * vNumCol * vCellSize + k) * 4] % 2 << " ";
        }
        std::cout << std::endl;
    }*/

    CIOImage Grid;
    Grid.setImageSize(vNumCol * vCellSize, vNumRow * vCellSize);
    Grid.setData(pData);

    return Grid;
}

void readObj(std::string vFileName, CVulkanRenderer& voRenderer)
{
    CIOObj Obj = CIOObj();
    Obj.read(vFileName);

    S3DObject ObjObject;
    ObjObject.TexIndex = 0;
    ObjObject.Vertices = Obj.getVertices();
    ObjObject.Normals = Obj.getNormalPerVertex();
    ObjObject.TexCoords = Obj.getRandomTexCoordPerVertex();
    for (size_t i = 0; i < ObjObject.Vertices.size(); ++i)
    {
        ObjObject.Colors.emplace_back(glm::vec3(1.0, 0.0, 0.0));
    }

    const std::vector<SObjFace>& Faces = Obj.getFaces();
    for (const SObjFace& Face : Faces)
    {
        uint32_t Node1 = Face.Nodes[0].VectexId - 1;
        for (size_t k = 2; k < Face.Nodes.size(); ++k)
        {
            uint32_t Node2 = Face.Nodes[k - 1].VectexId - 1;
            uint32_t Node3 = Face.Nodes[k].VectexId - 1;
            ObjObject.Indices.emplace_back(Node1);
            ObjObject.Indices.emplace_back(Node2);
            ObjObject.Indices.emplace_back(Node3);
        }
    }

    std::vector<S3DObject> SceneObjects;
    SceneObjects.emplace_back(ObjObject);

    std::vector<CIOImage> TexImages;
    TexImages.push_back(generateBlackPurpleGrid(4, 4, 16));

    voRenderer.setTextureImageData(TexImages);
    voRenderer.setSceneObjects(SceneObjects);
}

void readMap(std::string vFileName, CVulkanRenderer& voRenderer)
{
    CIOGoldSrcMap Map = CIOGoldSrcMap(vFileName);
    Map.read();
    std::vector<std::string> WadPaths = Map.getWadPaths();
    std::vector<CIOGoldsrcWad> Wads(WadPaths.size());
    
    for (size_t i = 0; i < WadPaths.size(); ++i)
    {
        std::string RealWadPath;
        if (!findFile(WadPaths[i], "../data", RealWadPath))
            throw "can't find wad file " + WadPaths[i];
        Wads[i].read(RealWadPath);
    }

    std::vector<CIOImage> TexImages;
    std::map<std::string, uint32_t> TexIndexMap;
    std::set<std::string> UsedTextureNames = Map.getUsedTextureNames();
    for (const std::string& TexName : UsedTextureNames)
    {
        for (const CIOGoldsrcWad& Wad : Wads)
        {
            std::optional<size_t> Index = Wad.findTexture(TexName);
            if (Index.has_value())
            {
                uint32_t Width = 0, Height = 0;
                Wad.getTextureSize(Index.value(), Width, Height);

                CIOImage TexImage;
                TexImage.setImageSize(static_cast<int>(Width), static_cast<int>(Height));
                void* pData = new char[Width * Height * 4];
                Wad.getRawRGBAPixels(Index.value(), pData);
                TexImage.setData(pData);
                delete[] pData;
                TexImages.emplace_back(TexImage);
                TexIndexMap[TexName] = TexIndexMap.size();
                break;
            }
        }
        if (TexIndexMap.find(TexName) == TexIndexMap.end())
            TexIndexMap[TexName] = 0;
    }

    
    // group polygon by texture, one object per texture 
    std::vector<S3DObject> SceneObjects(UsedTextureNames.size());
    for (size_t i = 0; i < SceneObjects.size(); ++i)
        SceneObjects[i].TexIndex = i;

    std::vector<CMapPolygon> Polygons = Map.getAllPolygons();
    for (CMapPolygon& Polygon : Polygons)
    {
        size_t TexIndex = TexIndexMap[Polygon.pPlane->TextureName];
        size_t TexWidth = TexImages[TexIndex].getImageWidth();
        size_t TexHeight = TexImages[TexIndex].getImageHeight();
        S3DObject& Object = SceneObjects[TexIndex];

        std::vector<glm::vec2> TexCoords = Polygon.getTexCoords(TexWidth, TexHeight);
        glm::vec3 Normal = Polygon.getNormal();

        Object.Vertices.insert(Object.Vertices.end(), Polygon.Vertices.begin(), Polygon.Vertices.end());
        Object.TexCoords.insert(Object.TexCoords.end(), TexCoords.begin(), TexCoords.end());
        for (size_t i = 0; i < Polygon.Vertices.size(); ++i)
        {
            Object.Colors.emplace_back(glm::vec3(1.0, 0.0, 0.0));
            Object.Normals.emplace_back(Normal);
        }

        uint32_t IndexStart = static_cast<uint32_t>(Object.Indices.size());
        for (size_t i = 1; i < Polygon.Vertices.size(); ++i)
        {
            Object.Indices.emplace_back(IndexStart);
            Object.Indices.emplace_back(IndexStart + i - 1);
            Object.Indices.emplace_back(IndexStart + i);
        }
    }

    voRenderer.setTextureImageData(TexImages);
    voRenderer.setSceneObjects(SceneObjects);
}

bool findFile(std::string vFilePath, std::string vSearchDir, std::string& voFilePath)
{
    std::filesystem::path CurPath(vFilePath);
    
    std::filesystem::path FullPath = std::filesystem::absolute(vFilePath);
    std::filesystem::path CurDir = FullPath.parent_path();
    while (!CurDir.empty() && CurDir != CurDir.parent_path())
    {
        std::filesystem::path SearchPath = std::filesystem::relative(FullPath, CurDir);
        std::filesystem::path CombinedSearchPath = std::filesystem::path(vSearchDir) / SearchPath;
        if (std::filesystem::exists(SearchPath))
        {
            voFilePath = SearchPath.string();
            return true;
        }
        else if (std::filesystem::exists(CombinedSearchPath))
        {
            voFilePath = CombinedSearchPath.string();
            return true;
        }
        CurDir = CurDir.parent_path();
    }

    return false;
}