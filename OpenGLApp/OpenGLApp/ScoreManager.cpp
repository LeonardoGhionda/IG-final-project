// ScoreManager.cpp
#include "ScoreManager.h"
#include "focusbox.h"
#include "ingredient.h"   
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
                                std::vector<std::string>& ingredientIds,   // <---
                                std::vector<SlashEffect>& activeCuts,
                                const Screen& screen,
                                double now,
                                GameState& gameState)
{
    if (trail.size() < 2) return;
    const float radius = 40.0f;

    for (auto it = ingredients.begin(); it != ingredients.end(); /* ++it gestito sotto */) {
        glm::vec2 objPos = it->MCSPositionOrtho(projection, view);

        bool hit = false;
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
                SlashEffect fx{ a, b, now };
                activeCuts.push_back(fx);
                break;
            }
        }

        if (hit) {
            glm::vec2 rectSize01 = focusBox.getScaledSize() * 2.0f;
            rectSize01.x /= static_cast<float>(screen.w);
            rectSize01.y /= static_cast<float>(screen.h);

            glm::vec2 rectCenter01 = focusBox.GetCenter();
            float rectMinX = glm::max(0.0f, rectCenter01.x - rectSize01.x * 0.5f);
            float rectMaxX = glm::min(1.0f, rectCenter01.x + rectSize01.x * 0.5f);

            float rectCenterY01 = 1.0f - rectCenter01.y;
            float rectMinY = glm::max(0.0f, rectCenterY01 - rectSize01.y * 0.5f);
            float rectMaxY = glm::min(1.0f, rectCenterY01 + rectSize01.y * 0.5f);

            const float objX01 = objPos.x / static_cast<float>(screen.w);
            const float objY01 = 1.0f - (objPos.y / static_cast<float>(screen.h));
            const bool inFocus =
                (objX01 >= rectMinX && objX01 <= rectMaxX) &&
                (objY01 >= rectMinY && objY01 <= rectMaxY);

            // indice corrente per leggere/raschiare l'ID parallelo
            const size_t idx = static_cast<size_t>(std::distance(ingredients.begin(), it));
            const std::string id = (idx < ingredientIds.size()) ? ingredientIds[idx] : std::string{};

            // --- REGOLE DI PUNTEGGIO/VITE ---
            if (it->IsBomb()) {
                // Bomba: vita -1 ovunque
                loseLife(1, gameState);
            }
            else if (id == "lim_marcio" || id == "cioc_marcio") {
                // Marci: -2 SEMPRE (anche fuori focus)
                addScore(-2);
            }
            else if (id == "mela_oro" || id == "fragola_oro") {
                // Oro: +3 SOLO se in focus, altrimenti -1
                if (inFocus) addScore(+3);
                else addScore(-1);
            }
            else if (id == "heart") {
                // Cuore: +1 vita SOLO se in focus, altrimenti nessun effetto
                if (inFocus) addLife(1);  // clamp interno a MAX_LIVES
            } else {
                if (inFocus) {
                    if (isRequiredId(id)) {
                        addScore(+1);
                        int& c = m_collected[id];
                        if (c < getRequiredQty(id)) c++;
                        if (isRecipeComplete()) {
                            gameState = GameState::CONGRATULATIONS;
                        }
                    } else {
                        addScore(-1);
                    }
                } else {
                    addScore(-1);
                }
            }

            // rimuovi sia l'ingrediente sia il suo ID parallelo
            it = ingredients.erase(it);
            if (idx < ingredientIds.size()) {
                ingredientIds.erase(ingredientIds.begin() + idx);
            }
        } else {
            ++it;
        }
    }
}


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