#pragma once

#include "model.h"
#include "shader.h"
#include "vecplus.h"
#include "screen.h"

//#define G glm::vec2(0.0f, -3.0f)
#define G glm::vec2(0.0f, -6.0f)


class Ingredient {

public:
    Ingredient(const char* path, glm::vec2 spawnpoint,float scale=1.0f, bool isBomb = false) : model(path), mat(1.0f), scaleFactor(scale), isBomb_(isBomb) {
        
      
        mat = glm::translate(mat, glm::vec3(spawnpoint, 0.0f));
        this->spawnpoint = spawnpoint;
        position = spawnpoint;

        mat = glm::scale(mat, glm::vec3(1.0f));	
        time = static_cast<float>(glfwGetTime());

        center = model.getModelAveragePosition();

        //get a direction towards the center shifted by a small random angle 
        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<float> angle(-15.0f, 15.0f);
        this->directionToCenter = RotateVec2(glm::normalize(-spawnpoint), glm::radians(angle(gen)));

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

    glm::mat4 GetModelMatrix() const {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
        transform = glm::scale(transform, glm::vec3(scaleFactor));
        return transform;
    }
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

    void SetVelocity(const glm::vec2& vel) {
        velocity = vel;
    }


    //center position in world space 
    glm::vec3 Position() const {
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
        screenPos.x = (normalizedCS.x + 1.0f) * 0.5f * screen.w + screen.paddingW;
        screenPos.y = (1.0f - (normalizedCS.y + 1.0f) * 0.5f) * screen.h + screen.paddingH;
        return screenPos;
    }

    glm::vec2 MCSPositionOrtho(const glm::mat4& projection, const glm::mat4& view) const {
        glm::vec4 clipSpace = projection * view * glm::vec4(Position(), 1.0f);
        glm::vec3 ndc = glm::vec3(clipSpace) / clipSpace.w;

        // Coordinate in pixel (da -1..1 a 0..screen.w/h)
        glm::vec2 screenPos;
        screenPos.x = (ndc.x * 0.5f + 0.5f) * screen.w; 
        screenPos.y = (ndc.y * 0.5f + 0.5f) * screen.h;

        return screenPos;
    }


    bool hit(glm::vec2 mousePos, glm::mat4 projection, glm::mat4 view) {
        glm::vec2 pos = MCSPosition(projection, view);
        std::cout << "mouse"<< mousePos <<std::endl;
        std::cout << "object"<< pos<<std::endl;
        bool hit =  glm::distance(mousePos, pos) <= CHB_MCS(projection, view)*1.5f;
        return hit;
    }
    static glm::vec2 RandomSpawnPoint() {
        std::random_device rd;
        std::mt19937 gen(rd());

        // Spawn solo dal basso dello schermo (asse y negativo)
        std::uniform_real_distribution<float> distX(-screen.screenlimit.x * 0.8f, screen.screenlimit.x * 0.8f);
        float x = distX(gen);
        float y = -screen.screenlimit.y - 1.0f; // un po' fuori schermo in basso

        return glm::vec2(x, y);
    }


    glm::vec2 getDirectionToCenter() {
        return this->directionToCenter;
    }

    void updateTime() {
        time = static_cast<float>(glfwGetTime());
    }

    Texture* getFirstTexture() {
        if (model.textures_loaded.size() > 0)
            return &model.textures_loaded[0];
        return nullptr;
    }
  bool IsBomb() const { return isBomb_; }
private:
    float scaleFactor;
    Model model;
    glm::mat4 mat;
    float time;
    float CHB;         //radius of a circular hitbox
    float pCHB = -1;   //radius of a circular hitbox projcted in MCS 
    glm::vec3 center;  //average center position Model matrix independent
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 spawnpoint;
    glm::vec2 directionToCenter; //direction towards the center of the screen shifted by a small random angle
    bool isBomb_ = false;

    //circular hitbox in mouse coord system
    float CHB_MCS(glm::mat4 projection, glm::mat4 view) {

        if (pCHB > 0.0) return pCHB; //no need to compute again

        glm::vec4 center = projection * view * glm::vec4(Position(), 1.0f);
        glm::vec4 offset = projection * view * glm::vec4(Position() + glm::vec3(CHB, 0.0f, 0.0f), 1.0f);

        glm::vec2 screenCenter = glm::vec2(
            (center.x / center.w + 1.0f) * 0.5f * screen.w,
            (1.0f - (center.y / center.w + 1.0f) * 0.5f) * screen.h
        );

        glm::vec2 screenOffset = glm::vec2(
            (offset.x / offset.w + 1.0f) * 0.5f * screen.w,
            (1.0f - (offset.y / offset.w + 1.0f) * 0.5f) * screen.h
        );

        return glm::length(screenOffset - screenCenter);
    }

    
};