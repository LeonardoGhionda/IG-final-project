// ScoreManager.cpp
#include "ScoreManager.h"
#include "focusbox.h"
#include "ingredient.h"    // deve dichiarare MCSPositionOrtho(), IsBomb()
#include "Effects.h"
#include "screen.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

void ScoreManager::loseLife(int n, GameState& gameState) {
    m_lives = std::max(0, m_lives - n);
    if (m_lives == 0) {
        gameState = GameState::NAME_INPUT;
    }
}

void ScoreManager::processSlash(const std::vector<glm::vec2>& trail,
                                const glm::mat4& projection,
                                const glm::mat4& view,
                                FocusBox& focusBox,
                                std::vector<Ingredient>& ingredients,
                                std::vector<SlashEffect>& activeCuts,
                                const Screen& screen,
                                double now,
                                GameState& gameState)
{
    if (trail.size() < 2) return;

    // Raggio di hit test in pixel (come nel tuo codice)
    const float radius = 40.0f;

    for (auto it = ingredients.begin(); it != ingredients.end(); /* ++it gestito sotto */) {

        // Posizione oggetto in coordinate schermo (pixel)
        glm::vec2 objPos = it->MCSPositionOrtho(projection, view);

        bool hit = false;

        // Trova subito il primo segmento che colpisce
        for (size_t i = 1; i < trail.size(); ++i) {
            const glm::vec2 a  = trail[i - 1];
            const glm::vec2 b  = trail[i];
            const glm::vec2 ab = b - a;
            const glm::vec2 ap = objPos - a;

            const float ab2 = glm::dot(ab, ab);
            if (ab2 < 0.0001f) continue;

            const float t = glm::clamp(glm::dot(ap, ab) / ab2, 0.0f, 1.0f);
            const glm::vec2 closest = a + t * ab;
            const float dist = glm::length(closest - objPos);

            if (dist < radius) {
                hit = true;

                // Effetto taglio subito (niente secondo loop)
                SlashEffect fx;
                fx.start = a;
                fx.end   = b;
                fx.timestamp = now;          // passato da fuori (ex glfwGetTime())
                activeCuts.push_back(fx);
                break;
            }
        }

        if (hit) {
            // ---- Calcolo inFocus (0..1 come nel pass blur) ----
            glm::vec2 rectSize01 = focusBox.getScaledSize() * 2.0f;
            rectSize01.x /= static_cast<float>(screen.w);
            rectSize01.y /= static_cast<float>(screen.h);

            glm::vec2 rectCenter01 = focusBox.GetCenter();  // già 0..1
            float rectMinX = rectCenter01.x - rectSize01.x * 0.5f;
            float rectMaxX = rectCenter01.x + rectSize01.x * 0.5f;

            // inverti Y come nel blur
            float rectCenterY01 = 1.0f - rectCenter01.y;
            float rectMinY = rectCenterY01 - rectSize01.y * 0.5f;
            float rectMaxY = rectCenterY01 + rectSize01.y * 0.5f;

            // clamp ai bordi
            rectMinX = glm::max(0.0f, rectMinX);
            rectMinY = glm::max(0.0f, rectMinY);
            rectMaxX = glm::min(1.0f, rectMaxX);
            rectMaxY = glm::min(1.0f, rectMaxY);

            // posizione oggetto 0..1 (Y invertita)
            const float objX01 = objPos.x / static_cast<float>(screen.w);
            const float objY01 = 1.0f - (objPos.y / static_cast<float>(screen.h));

            const bool inFocus =
                (objX01 >= rectMinX && objX01 <= rectMaxX) &&
                (objY01 >= rectMinY && objY01 <= rectMaxY);

            // ---- punteggio / vite ----
            if (it->IsBomb()) {
                // bomba: vita -1 ovunque
                loseLife(1, gameState);
            } else {
                if (inFocus) {
                    addScore(+1);
                } else {
                    addScore(-1);
                }
            }

            // rimuovi l'ingrediente colpito
            it = ingredients.erase(it);
        } else {
            ++it;
        }
    }
}

// ------------------- le tue funzioni di persistenza -------------------

std::vector<ScoreEntry> ScoreManager::LoadScores(const std::string& filename) {
    std::vector<ScoreEntry> scores;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name;
        int s;
        if (std::getline(ss, name, ',') && (ss >> s)) {
            scores.push_back({ name, s });
        }
    }

    std::sort(scores.begin(), scores.end(),
        [](const ScoreEntry& a, const ScoreEntry& b) { return b.score < a.score; });

    if (scores.size() > 10) scores.resize(10);
    return scores;
}

void ScoreManager::SaveScore(const std::string& name, int score) {
    std::ofstream file("score.txt", std::ios::app);
    if (file.is_open()) {
        file << name << "," << score << "\n";
    } else {
        std::cerr << "Errore: impossibile aprire score.txt per scrittura.\n";
    }
}