#pragma once
#include <glm/glm.hpp>
#include "shader.h"

class FocusBox {
    public:
        FocusBox(const glm::vec2& size = glm::vec2(150.0f, 100.0f));

        void Move(const glm::vec2& offset);
        void SetCenter(const glm::vec2& pos);
        void Draw(const Shader& shader, int screenWidth, int screenHeight);
        bool Contains(const glm::vec2& screenPoint) const;
        glm::vec2 GetCenter() const;
        glm::vec2  GetSize() const;
        // Add this method to the FocusBox class declaration (public section)
        bool IsActive() const;
        void printOnClick();
        glm::vec2 getScaledSize();
        void resetSize();
        void increaseSize();
        void reduceSize();

    private:
        int size_cnt = 0;
        glm::vec2 center;
        glm::vec2 size, inital_size;
        unsigned int VAO, VBO;
        void InitRenderData();
};