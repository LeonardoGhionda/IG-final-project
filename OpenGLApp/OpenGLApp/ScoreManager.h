#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include "GameState.h"      // ok: enum/valori usati nel .h


class Ingredient;          // idem
class Screen;             // se è uno struct
struct SlashEffect;        // se è uno struct
class FocusBox;            // <-- forward decl

struct ScoreEntry {
    std::string name;
    int score;
};

class ScoreManager {
public:
    explicit ScoreManager(int lives = 3) : m_score(0), m_lives(lives) {}

    void reset(int lives = 3) { m_score = 0; m_lives = lives; }
    int  getScore() const { return m_score; }
    int  getLives() const { return m_lives; }

    void processSlash(const std::vector<glm::vec2>& trail,
                      const glm::mat4& projection,
                      const glm::mat4& view,
                      FocusBox& focusBox,
                      std::vector<Ingredient>& ingredients,
                      std::vector<SlashEffect>& activeCuts,
                      const Screen& screen,
                      double now,
                      GameState& gameState);

    static std::vector<ScoreEntry> LoadScores(const std::string& filename);
    static void SaveScore(const std::string& name, int score);

private:
    void addScore(int delta) { m_score = std::max(0, m_score + delta); }
    void loseLife(int n, GameState& gameState);

    int m_score{0};
    int m_lives{3};
};