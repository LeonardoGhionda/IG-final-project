#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include "shader.h"
#include "model.h"


struct Button {
public:
    Button(const char* path, std::string label, glm::vec3 scale): model(path), label(label), mat(1.0f) {}

    bool mouseInside(glm::vec2 mouse) const {
        glm::vec3 pos = glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f);
        glm::vec3 size = glm::vec3(screen.w / 3.2f / 8, screen.h / 1.8f / 8, 1.0f);

        float halfW = size.x;
        float halfH = size.y;

        float left = pos.x - halfW;
        float right = pos.x + halfW;
        float bottom = pos.y - halfH;
        float top = pos.y + halfH;

        /*
        printf("%.1f  |  %.1f\n", mouse.x, mouse.y);
        printf("%.1f  |  %.1f\n", left, right);
        printf("%.1f  |  %.1f\n\n\n", bottom, top);
        */

        return mouse.x >= left && mouse.x <= right &&
            mouse.y >= bottom && mouse.y <= top;
    }


    void Draw(Shader& shader) {
        model.Draw(shader);
    }

    glm::mat4 GetModelMatrix() { 
        mat = glm::mat4(1.0f);
        mat = glm::translate(mat, glm::vec3(screen.w / 2.0f, screen.h / 2.0f, 0.0f));
        mat = glm::scale(mat, glm::vec3(screen.w / 3.2f / 8, screen.h / 1.8f / 8, 1.0f));
        return mat; 
    }

private:
    std::string label;
    glm::mat4 mat;
    Model model;
};