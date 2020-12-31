#include "Interactor.h"
#include "VulkanRenderer.h"

#include <GLFW/glfw3.h>
#include <chrono>
#include <filesystem>
#include <set>
#include "IOGoldSrcMap.h"
#include "IOGoldSrcWad.h"
#include "IOImage.h"

void readObj(std::string vFileName, CVulkanRenderer& voRenderer);
void readMap(std::string vFileName, CVulkanRenderer& voRenderer);

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	GLFWwindow* pWindow = glfwCreateWindow(800, 600, "Vulkan Simple Render", nullptr, nullptr);
	CVulkanRenderer Renderer(pWindow);
	Renderer.getCamera()->setPos(glm::vec3(0.0f, 0.0f, 3.0f));
    Renderer.init(); 

    readMap("../data/test.map", Renderer);

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

void readObj(std::string vFileName, CVulkanRenderer& voRenderer)
{
    CIOObj Obj = CIOObj();
    Obj.read(vFileName);

    S3DObject ObjObject;
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
        Wads[i].read(WadPaths[i]);
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
                void* pData;
                Wad.getRawRGBAPixels(Index.value(), pData);
                TexImage.setData(pData);
                delete pData;
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
        S3DObject& Object = SceneObjects[TexIndex];

        std::vector<glm::vec2> TexCoords = Polygon.getTexCoords();
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

    voRenderer.setSceneObjects(SceneObjects);
}