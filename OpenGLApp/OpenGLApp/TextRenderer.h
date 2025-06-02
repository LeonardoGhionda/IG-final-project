#pragma once
#include <map>
#include <string>
#include <glm/glm.hpp>
#include "shader.h"

struct Character {
    unsigned int TextureID;
    glm::ivec2 Size;
    glm::ivec2 Bearing;
    unsigned int Advance;
};

class TextRenderer {
public:
    std::map<char, Character> Characters;
    unsigned int VAO, VBO;

    bool Load(std::string fontPath, unsigned int fontSize);
    void DrawText(Shader& shader, std::string text, float x, float y, float scale, glm::vec3 color);
};
