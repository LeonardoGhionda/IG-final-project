#pragma once

#include "model.h"
#include "shader.h"
#include "vecplus.h"
#include "screen.h"
#include <GLFW/glfw3.h>

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

        std::random_device rd;
        std::mt19937 gen(rd());

        std::uniform_real_distribution<float> angle(-15.0f, 15.0f);
        this->directionToCenter = RotateVec2(glm::normalize(-spawnpoint), glm::radians(angle(gen)));

        float CHBSq = 0.0f;
        for (const Mesh& m : model.meshes) {
            for (const Vertex& v : m.vertices) {
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
       // mat = glm::translate(glm::mat4(1.0f), glm::vec3(position, 0.0f));
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


    glm::vec3 Position() const {
        glm::vec3 scaledCenter = glm::vec3(center) * scaleFactor;
        return glm::vec3(position, 0.0f) + scaledCenter;
    }


    glm::vec2 MCSPosition(glm::mat4 projection, glm::mat4 view) {

        // M -> V -> P
        glm::vec4 clipSpace = projection * view * glm::vec4(Position(), 1.0f);

        glm::vec3 normalizedCS = glm::vec3(clipSpace) / clipSpace.w;

      
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
    static glm::vec2 RandomSpawnPoint(float margin = 60.0f) {
        // RNG persistente (evita di reinizializzarlo ad ogni chiamata)
        static thread_local std::mt19937 gen(std::random_device{}());

        // Lato di spawn: 0=sinistra, 1=destra, 2=basso, 3=alto
        std::uniform_int_distribution<int> side(0, 3);
        std::uniform_real_distribution<float> rx(0.0f, (float)screen.w);
        std::uniform_real_distribution<float> ry(0.0f, (float)screen.h);

        switch (side(gen)) {
            case 0:  // sinistra
                return glm::vec2(-margin, ry(gen));
            case 1:  // destra
                return glm::vec2(screen.w + margin, ry(gen));
            case 2:  // basso
                return glm::vec2(rx(gen), -margin);
            default: // alto
                return glm::vec2(rx(gen), screen.h + margin);
        }
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
    float CHB;       
    float pCHB = -1;  
    glm::vec3 center;  
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 spawnpoint;
    glm::vec2 directionToCenter; 
    bool isBomb_ = false;

    
    float CHB_MCS(glm::mat4 projection, glm::mat4 view) {

        if (pCHB > 0.0) return pCHB;

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