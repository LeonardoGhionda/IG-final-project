#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>              // <--- AGGIUNTO
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include "GameState.h"

class Ingredient;
class Screen;
struct SlashEffect;
class FocusBox;

struct ScoreEntry {
    std::string name;
    int score;
};

class ScoreManager {
public:
    explicit ScoreManager(int lives = 3) : m_score(0), m_lives(lives) {}

    void reset(int lives = 3) { m_score = 0; m_lives = lives; m_collected.clear(); }
    int  getScore() const { return m_score; }
    int  getLives() const { return m_lives; }

    // --- NUOVO: definisci la ricetta (ID -> quantità)
    void setRequiredRecipe(const std::unordered_map<std::string,int>& req) {
        m_required = req;
        m_collected.clear();
    }
    int getRequiredQty(const std::string& id) const {
        auto it = m_required.find(id);
        return it == m_required.end() ? 0 : it->second;
    }
    int getCollectedQty(const std::string& id) const {
        auto it = m_collected.find(id);
        return it == m_collected.end() ? 0 : it->second;
    }

    // --- CAMBIATA: passiamo anche gli ID paralleli
    void processSlash(const std::vector<glm::vec2>& trail,
                      const glm::mat4& projection,
                      const glm::mat4& view,
                      FocusBox& focusBox,
                      std::vector<Ingredient>& ingredients,
                      std::vector<std::string>& ingredientIds,   // <--- AGGIUNTO
                      std::vector<SlashEffect>& activeCuts,
                      const Screen& screen,
                      double now,
                      GameState& gameState);

    static std::vector<ScoreEntry> LoadScores(const std::string& filename);
    static void SaveScore(const std::string& name, int score);

private:
    void addScore(int delta) { m_score = std::max(0, m_score + delta); }
    void loseLife(int n, GameState& gameState);

    bool isRequiredId(const std::string& id) const {
        return m_required.find(id) != m_required.end();
    }
    bool isRecipeComplete() const {
        for (const auto& kv : m_required) {
            auto it = m_collected.find(kv.first);
            if (it == m_collected.end() || it->second < kv.second) return false;
        }
        return true;
    }

    int m_score{0};
    int m_lives{3};
    std::unordered_map<std::string,int> m_required;   // ID -> qty richiesta
    std::unordered_map<std::string,int> m_collected;  // ID -> qty raccolta
};