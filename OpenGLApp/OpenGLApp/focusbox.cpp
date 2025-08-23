#include "focusbox.h"
#include <glad/glad.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <iostream>

#include "screen.h"
extern Screen screen;

FocusBox::FocusBox(const glm::vec2& size) : size(size), center(0.0f, 0.0f) {
    InitRenderData();
}

void FocusBox::InitRenderData() {
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void FocusBox::Move(const glm::vec2& offset) {
    center += offset;
}

void FocusBox::SetCenter(const glm::vec2& pos) {
    center = pos;
}

glm::vec2 FocusBox::GetCenter() const {
    return center;
}

glm::vec2 FocusBox::GetSize() const {
    return size;
}

bool FocusBox::Contains(const glm::vec2& screenPoint) const {
    return screenPoint.x >= center.x - size.x && screenPoint.x <= center.x + size.x &&
        screenPoint.y >= center.y - size.y && screenPoint.y <= center.y + size.y;
}

void FocusBox::Draw(const Shader& shader, int screenWidth, int screenHeight) {
    shader.use();

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(screenWidth), 0.0f, static_cast<float>(screenHeight));
    glm::mat4 model = glm::mat4(1.0f);

    glm::vec2 pos =  glm::vec2(
        center.x * screen.w,
        center.y * screen.h
    );
    model = glm::translate(model, glm::vec3(pos, 0.0f));

    glm::vec2 screenScale = screen.scaleFactor();
    model = glm::scale(model, glm::vec3(size.x * screenScale.x, size.y * screenScale.y, 1.0f));

    shader.setMat4("projection", projection);
    shader.setMat4("model", model);
    shader.setVec3("diffuseColor", glm::vec3(0.0f, 0.0f, 1.0f));

    glLineWidth(5.0f);
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glBindVertexArray(0);
}
glm::vec2 FocusBox::GetPosition() const {
    return center;
}
bool FocusBox::IsActive() const{
    return true;
}

// debug
void FocusBox::printOnClick(){
    glm::vec2 pos = glm::vec2(
        center.x * screen.w,
        center.y * screen.h
    );
    std::cout << "box: " << pos.x << ", " << pos.y << std::endl;
}
