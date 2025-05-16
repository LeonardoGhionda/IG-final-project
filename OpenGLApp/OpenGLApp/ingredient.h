#pragma once

#include "model.h"
#include "shader.h"
#include "vecplus.h"

#define G glm::vec2(0.0f, -25.0f)


class Ingredient {
public:
    Ingredient(const char* path, glm::vec2 spawnpoint) : model(path), mat(1.0f) {
        mat = glm::translate(mat, glm::vec3(spawnpoint, 0.0f));
        position = spawnpoint;
        mat = glm::scale(mat, glm::vec3(1.0f));	
        time = static_cast<float>(glfwGetTime());

        center = model.getModelAveragePosition();

        float CHBSq = 0.0f;
        for (const Mesh& m : model.meshes) {
            for (const Vertex& v : m.vertices) {
                //search for max square distance (to use sqrt only once in the end)
                CHBSq = glm::max(glm::dot(v.Position, v.Position), CHBSq);
            }
        }
        CHB = std::sqrt(CHBSq);
        velocity = glm::vec2(0.0f);
    }

    void Draw(Shader shader) { 
        model.Draw(shader); 
    }

    glm::mat4 GetModelMatrix() { return mat; }

    void Move() {
        ApplyGravity();
        float deltaTime = static_cast<float>(glfwGetTime()) - time;
        position += velocity * deltaTime;
        mat = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
        updateTime();
    }

    void ApplyGravity() {
        float deltaTime = static_cast<float>(glfwGetTime()) - time;
        velocity += G * deltaTime;
    }

    void AddVelocity(glm::vec2 delta) {
        velocity += delta;
    }

    //center position in world space 
    glm::vec3 Position() {
        glm::vec4 centerWorldSpace = mat * glm::vec4(center, 1.0f);
        return glm::vec3(centerWorldSpace);
    }

    //Model center posizion in the mouse coordinate system
    glm::vec2 MCSPosition(glm::mat4 projection, glm::mat4 view) {

        // M -> V -> P
        glm::vec4 clipSpace = projection * view * glm::vec4(Position(), 1.0f);

        //(-1 , 1)
        glm::vec3 normalizedCS = glm::vec3(clipSpace) / clipSpace.w;

        // Screen-space (pixel,  0 - width/height)
        glm::vec2 screenPos;
        screenPos.x = (normalizedCS.x + 1.0f) * 0.5f * 1280;
        screenPos.y = (normalizedCS.y + 1.0f) * 0.5f * 720;
        //GLFW MCS has inverted y ax
        screenPos.y = 720 - screenPos.y;
        return screenPos;
    }

    bool hit(glm::vec2 mousePos, glm::mat4 projection, glm::mat4 view) {
        glm::vec2 pos = MCSPosition(projection, view);
        return glm::distance(mousePos, pos) <= CHB_MCS(projection, view);
    }

   
private:
    Model model;
    glm::mat4 mat;
    float time;
    float CHB; //radius of a circular hitbox
    float pCHB = -1; //radius of a circular hitbox projcted in MCS 
    glm::vec3 center;  //average center position Model matrix independent
    glm::vec2 position;
    glm::vec2 velocity;


    float CHB_MCS(glm::mat4 projection, glm::mat4 view) {

        if (pCHB > 0.0) return pCHB; //no need to compute again

        glm::vec4 center = projection * view * glm::vec4(Position(), 1.0f);
        glm::vec4 offset = projection * view * glm::vec4(Position() + glm::vec3(CHB, 0.0f, 0.0f), 1.0f);

        glm::vec2 screenCenter = glm::vec2((center.x / center.w + 1.0f) * 0.5f * 1280,
            (1.0f - (center.y / center.w + 1.0f) * 0.5f) * 720);
        glm::vec2 screenOffset = glm::vec2((offset.x / offset.w + 1.0f) * 0.5f * 1280,
            (1.0f - (offset.y / offset.w + 1.0f) * 0.5f) * 720);

        return glm::length(screenOffset - screenCenter);
    }

    void updateTime() {
        time = static_cast<float>(glfwGetTime());
    }
};