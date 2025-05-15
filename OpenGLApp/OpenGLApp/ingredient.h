#pragma once

#include "model.h"
#include "shader.h"


class Ingredient {
public:
    Ingredient(const char* path) : model(path), mat(1.0f) {
        mat = glm::translate(mat, glm::vec3(0.0f, 0.0f, 1.0f));
        mat = glm::scale(mat, glm::vec3(1.0f));	
        time = static_cast<float>(glfwGetTime());
    }

    void Draw(Shader shader) { model.Draw(shader); }

    glm::mat4 getModelMatrix() { return mat; }

    void move(glm::vec2 offset, float speed) {
        glm::vec3 offset3 = glm::normalize(glm::vec3(offset, 0.0f));
        float new_time = static_cast<float>(glfwGetTime());
        mat = glm::translate(mat, offset3 * speed * (new_time - time));
        time = new_time;
    }

private:
    Model model;
    glm::mat4 mat;
    float time;
};