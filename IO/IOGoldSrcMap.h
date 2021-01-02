#pragma once
#include "IOBase.h"

#include <vector>
#include <array>
#include <map>
#include <set>
#include <glm/glm.hpp>

struct CMapPlane
{
	std::array<glm::vec3, 3> Points;
	std::string TextureName;
	glm::vec3 TextureDirectionU;
	float TextureOffsetU;
	glm::vec3 TextureDirectionV;
	float TextureOffsetV;
	float TextureRotation;
	float TextureScaleU;
	float TextureScaleV;

	glm::vec3 getNormal() const;
	float getDistanceToFace(glm::vec3 vPoint = glm::vec3(0, 0, 0)) const;
	glm::vec3 correctNormal(glm::vec3 vN) const;
};

struct CMapPolygon
{
	std::vector<glm::vec3> Vertices;
	const CMapPlane* pPlane;

	std::vector<glm::vec3> getVertices() const { return Vertices; }
	std::vector<glm::vec2> getTexCoords(size_t vTexWidth, size_t vTexHeight);
	glm::vec3 getNormal() const { return pPlane->getNormal(); };

private:
	glm::vec2 __calcTexCoord(glm::vec3 vVertex, size_t vTexWidth, size_t vTexHeight);
	std::vector<glm::vec2> m_TexCoords;
};

struct CMapBrush
{
	std::vector<CMapPlane> Planes;

	std::vector<CMapPolygon> getPolygons();
	
	static void sortVerticesInClockwise(std::vector<glm::vec3>& vVertices, const glm::vec3 vNormal);
	
private:
	bool __getIntersection(glm::vec3& voPoint, size_t vPlane1, size_t vPlane2, size_t vPlane3);
	
	std::vector<CMapPolygon> m_Polygons;
};

struct CMapEntity
{
	std::map<std::string, std::string> Properties;
	std::vector<CMapBrush> Brushes;

	void read(std::string vTextEntity);
private:
	void __readProperties(std::string vTextProperties);
	void __readProperty(std::string vTextProperty);
	void __readBrushes(std::string vTextBrushes);
	void __readBrush(std::string vTextBrush);
	CMapPlane __parsePlane(std::string vTextPlane);
	std::vector<float> __parseFloatArray(std::string vText);
	float __parseFloat(std::string vText);
};

/***************************************************************
 * Class for map file (valve hammer editor source file which contain brush and entity) reading.
 * Here is the basic structure of map file:
 * map is an ascii type file (i.e. text file).
 * map is composed with multiple "entities", which is surounded by a pair of brace {}.
 * An entity may contain properties and brush data.
 * Each property use the key-value form. One property cover a whole line. Key and value are in two double quotation marks separated by a white space (e.g. "classname" "worldspawn")
 * Brush data discribe brush of this entity. Entity may have multiple brushes.
 * Each brushes is surounded by a pair of brace {}, too.
 * A brush contains many faces, and each face use one line. The structure of this brush is calculated by finding intersection of those faces (which is pretty strange in my opinion)
 * Each face look like this:
 * (x1 y1 z1) (x2 y2 z2) (x3 y3 z3) TexName [ ux uy uz uOffset ] [ vx vy vz vOffset ] rotate uScale vScale
 * The first 3 points define the face, then comes the texture name. Next is the uv direction and offset, followed by rotate angle and uv scale
 * If you want to know more about map, like how to compute the actual uv mapping, this ariticle is very useful: https://github.com/stefanha/map-files 
 **************************************************************/
class CIOGoldSrcMap : public CIOBase
{
public:
	CIOGoldSrcMap() :CIOBase() {}
	CIOGoldSrcMap(std::string vFileName) :CIOBase(vFileName) {}

	std::vector<std::string> getWadPaths();
	const std::vector<CMapEntity>& getEntities() { return m_Entities; }
	std::set<std::string> getUsedTextureNames();
	std::vector<CMapPolygon> getAllPolygons();
	std::string toString();

protected:
	virtual bool _readV(std::string vFileName) override;

private:
	std::vector<CMapEntity> m_Entities;
};