#pragma once
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace UI
{
    void text(std::string vName);
    bool input(std::string vName, float& vioValue, float vStep = 0.01f);
    bool drag(std::string vName, float& vioValue, float vMin = 0.0f, float vMax = 1.0f, float vStep = 0.01f);
    bool inputColor(std::string vName, glm::vec3& vioColor);
    bool inputColor(std::string vName, glm::vec4& vioColor);
    bool toggle(std::string vName, bool& vioValue);
    void indent(float vWidth);
    void split();
    bool isUsingMouse();
    bool isUsingKeyboard();
};

