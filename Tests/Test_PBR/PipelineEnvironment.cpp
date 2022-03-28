#include "PipelineEnvironment.h"
#include "Function.h"

struct SUBOVert
{
    glm::mat4 Proj;
    glm::mat4 View;
    glm::mat4 Model;
};

struct SUBOFrag
{
    glm::vec4 Eye;
    uint32_t ForceUseMat = 0u;
    uint32_t UseColorTexture = 1u;
    uint32_t UseNormalTexture = 1u;
    uint32_t UseSpecularTexture = 1u;
};