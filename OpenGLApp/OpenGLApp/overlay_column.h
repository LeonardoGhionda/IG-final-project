#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

#include "shader.h"

class OverlayColumn {
public:
    OverlayColumn(float x, float width, float height)
        : x(x), width(width), height(height), active(false) {
        float vertices[] = {
            x, 0.0f,
            x + width, 0.0f,
            x + width, height,
            x, height
        };
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    void Draw(const Shader& shader, const glm::mat4& projection) const {
        if (active) return;

        shader.use();
        shader.setMat4("projection", projection);
        shader.setVec4("overlayColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.75f));

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void SetActive(bool value) {
        active = value;
    }

private:
    float x, width, height;
    bool active;
    GLuint vao, vbo;
};




class OverlayManager {
public:
    OverlayManager(float screenW, float screenH)
        : screenWidth(screenW), screenHeight(screenH) {
        float colWidth = screenW / 3.0f;
        for (int i = 0; i < 3; ++i) {
            columns.push_back(OverlayColumn(i * colWidth, colWidth, screenH));
        }
        SetActiveColumn(1);
    }

    void SetActiveColumn(int index) {
        for (int i = 0; i < 3; ++i) {
            columns[i].SetActive(i == index);
        }
        activeIndex = index;
    }

    void SetActiveLeft() {
        if (activeIndex > 0) SetActiveColumn(activeIndex - 1);
    }
    void SetActiveRight() {
        if (activeIndex < 2) SetActiveColumn(activeIndex + 1);
    }

    void Draw(const Shader& shader) const {
        glm::mat4 projection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
        for (const auto& col : columns) {
            col.Draw(shader, projection);
        }
    }

private:
    std::vector<OverlayColumn> columns;
    float screenWidth, screenHeight;
    unsigned short int activeIndex;
};