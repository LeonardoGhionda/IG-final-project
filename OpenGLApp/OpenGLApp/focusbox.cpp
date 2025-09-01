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

static inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

void FocusBox::Move(const glm::vec2& offset) {
    center += offset;
    // half-size in pixel (già scalato rispetto allo schermo)
    glm::vec2 halfPx = getScaledSize();
    // converti half-size in coordinate normalizzate
    glm::vec2 halfNdc = glm::vec2(
        halfPx.x / static_cast<float>(screen.w),
        halfPx.y / static_cast<float>(screen.h)
    );

    center.x = clampf(center.x, halfNdc.x, 1.0f - halfNdc.x);
    center.y = clampf(center.y, halfNdc.y, 1.0f - halfNdc.y);
}

void FocusBox::SetCenter(const glm::vec2& pos) {
    center = pos;
    glm::vec2 halfPx = getScaledSize();
    glm::vec2 halfNdc = glm::vec2(
        halfPx.x / static_cast<float>(screen.w),
        halfPx.y / static_cast<float>(screen.h)
    );

    center.x = clampf(center.x, halfNdc.x, 1.0f - halfNdc.x);
    center.y = clampf(center.y, halfNdc.y, 1.0f - halfNdc.y);
}

glm::vec2 FocusBox::GetCenter() const {
    return center;
}

glm::vec2 FocusBox::GetSize() const {
    return size;
}

bool FocusBox::Contains(const glm::vec2& screenPoint) const {
    // centro in pixel
    glm::vec2 centerPx = glm::vec2(center.x * screen.w, center.y * screen.h);
    // half-size in pixel (già scalato)
    glm::vec2 halfPx = const_cast<FocusBox*>(this)->getScaledSize();
    // center e size sono in pixel (size = semi-lati)
    return (screenPoint.x >= centerPx.x - halfPx.x && screenPoint.x <= centerPx.x + halfPx.x) &&
        (screenPoint.y >= centerPx.y - halfPx.y && screenPoint.y <= centerPx.y + halfPx.y);
}

void FocusBox::Draw(const Shader& shader, int screenWidth, int screenHeight) {
    shader.use();

    // Ortho in pixel
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, 0.0f, (float)screenHeight);

    // Unica TRASLAZIONE in pixel (niente * screen.w/h)
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(center * glm::vec2(screenWidth, screenHeight), 0.0f));

    
    model = glm::scale(model, glm::vec3(getScaledSize(), 1.0f));

    shader.setMat4("projection", projection);
    shader.setMat4("model", model);
    shader.setVec3("diffuseColor", glm::vec3(0.0f, 0.0f, 0.0f));

    glLineWidth(1.0f);
    glBindVertexArray(VAO);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glBindVertexArray(0);
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

glm::vec2 FocusBox::getScaledSize()
{
    glm::vec2 screenScale = screen.scaleFactor();
    return glm::vec2(size.x * screenScale.x, size.y * screenScale.y);
}
