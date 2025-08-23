#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>

#include "shader.h"
#include "model.h"

#include "Screen.h"
extern Screen screen;

struct Button {
    //pos changed: defined as screen percentage
    glm::vec2 pos;

    glm::vec2 size;
    float yScale;
    std::string label;

    bool isClicked(glm::vec2 mouse) {
        glm::vec2 pixelPos = Pos();

        glm::vec2 screenScale = screen.scaleFactor();
        float halfWidth = (size.x * screenScale.x) / 2.0f;
        float halfHeight = (size.y * screenScale.y * yScale) / 2.0f;

        return mouse.x >= pixelPos.x - halfWidth && mouse.x <= pixelPos.x + halfWidth &&
            mouse.y >= pixelPos.y - halfHeight && mouse.y <= pixelPos.y + halfHeight;
    }

    void Draw(Shader& shader, Model& model) {
        glm::mat4 mat = glm::mat4(1.0f);
        mat = glm::translate(mat, glm::vec3(Pos(), 0.0f));
        glm::vec2 screenScale = screen.scaleFactor();
        mat = glm::scale(mat, glm::vec3(size.x * screenScale.x, size.y * screenScale.y, 1.0f));
        shader.setMat4("model", mat);
        model.Draw(shader);
    }

    glm::vec2 Pos() {
        return glm::vec2(
            pos.x * screen.w,
            pos.y * screen.h
        );
    }
};