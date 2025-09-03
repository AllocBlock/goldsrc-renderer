#pragma once
#include "Pointer.h"
#include "IOImage.h"
#include "SceneInfo.h"

#include <glm/glm.hpp>
#include <vector>
#include <filesystem>

struct SObjObject
{
    std::string Name;
    sptr<IDataArray<glm::vec3>> pVertexArray;
    sptr<IDataArray<glm::vec2>> pTexCoordArray;
    sptr<CIOImage> pImage;
};

/* e.g.
.obj:

mtllib MY_MODEL.mtl

g GROUP_1
usemtl TEST_1
v -0.50000000 -0.50000000 -0.50000000
v -0.50000000 -0.50000000 0.50000000
v -0.50000000 0.50000000 -0.50000000
v -0.50000000 0.50000000 0.50000000
v 0.50000000 -0.50000000 -0.50000000
v 0.50000000 -0.50000000 0.50000000
v 0.50000000 0.50000000 -0.50000000
v 0.50000000 0.50000000 0.50000000
vt 0.000000 0.000000
vt 1.000000 0.000000
vt 0.000000 1.000000
vt 1.000000 1.000000
f 1/1 3/2 7/4 5/3
f 3/1 4/2 8/4 7/3
f 5/1 7/2 8/4 6/3


g GROUP_2
usemtl TEST_2
v -0.50000000 -0.50000000 -0.50000000
v -0.50000000 -0.50000000 0.50000000
v -0.50000000 0.50000000 -0.50000000
vt 0.000000 0.000000
vt 1.000000 0.000000
vt 0.000000 1.000000
f 9/5 10/6 11/7
.mtl

newmtl MY_MTL_1
Ka 1 1 1
Kd 1 1 1
d 1
Ns 0
illum 1
map_Kd TEST_1.jpg

newmtl MY_MTL_2
Ka 1 1 1
Kd 1 1 1
d 1
Ns 0
illum 1
map_Kd TEST_2.jpg
*/

class CSceneObjWriter
{
public:
    void addSceneInfo(sptr<SSceneInfo> vSceneInfo);
    void addObject(sptr<SObjObject> vObject);
    void writeToFile(std::filesystem::path vFilePath);

private:
    std::vector<sptr<SObjObject>> m_ObjectSet;
};
