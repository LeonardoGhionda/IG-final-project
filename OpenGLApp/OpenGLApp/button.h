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
    // posizione in percentuale dello schermo (0..1)
    glm::vec2 pos;

    // dimensione base (in “unità modello”, poi scalata per lo schermo)
    glm::vec2 size;

    // scala verticale extra (per renderli più alti)
    float yScale = 1.0f;

    std::string label;

    // fattore di hover (animato)
    float hoverScale = 1.0f;

    // Aggiorna l'hover in base al cursore; da chiamare ogni frame prima di Draw()
    void UpdateHover(glm::vec2 mouse) {
        const bool over = isHovered(mouse);
        const float target = over ? 1.08f : 1.0f;  // quanto ingrandire
        const float smooth = 0.2f;                 // 0..1 (più alto = più reattivo)
        hoverScale += (target - hoverScale) * smooth;
    }

    // Test hover con dimensioni ATTUALI (include hoverScale)
    bool isHovered(glm::vec2 mouse) const {
        glm::vec2 pixelPos = Pos();
        glm::vec2 screenScale = screen.scaleFactor();

        float halfWidth  = (size.x * screenScale.x * hoverScale) / 2.0f;
        float halfHeight = (size.y * screenScale.y * yScale * hoverScale) / 2.0f;

        return  mouse.x >= pixelPos.x - halfWidth  && mouse.x <= pixelPos.x + halfWidth &&
                mouse.y >= pixelPos.y - halfHeight && mouse.y <= pixelPos.y + halfHeight;
    }

    // Click test: uso le dimensioni attuali (così “clicchi quello che vedi”)
    bool isClicked(glm::vec2 mouse) const {
        glm::vec2 pixelPos = Pos();
        glm::vec2 screenScale = screen.scaleFactor();

        float halfWidth  = (size.x * screenScale.x * hoverScale) / 2.0f;
        float halfHeight = (size.y * screenScale.y * yScale * hoverScale) / 2.0f;

        return  mouse.x >= pixelPos.x - halfWidth  && mouse.x <= pixelPos.x + halfWidth &&
                mouse.y >= pixelPos.y - halfHeight && mouse.y <= pixelPos.y + halfHeight;
    }

    // Disegno: applico sia yScale sia hoverScale
    void Draw(Shader& shader, Model& model) const {
        glm::mat4 mat(1.0f);
        mat = glm::translate(mat, glm::vec3(Pos(), 0.0f));

        glm::vec2 screenScale = screen.scaleFactor();
        float sx = size.x * screenScale.x * hoverScale;
        float sy = size.y * screenScale.y * yScale * hoverScale;

        mat = glm::scale(mat, glm::vec3(sx, sy, 1.0f));
        shader.setMat4("model", mat);
        model.Draw(shader);
    }

    glm::vec2 Pos() const {
        return { pos.x * screen.w, pos.y * screen.h };
    }
};
