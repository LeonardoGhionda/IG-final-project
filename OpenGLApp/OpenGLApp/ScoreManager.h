#pragma once
#include <string>
#include <vector>

struct ScoreEntry {
    std::string name;
    int score;
};

class ScoreManager {
public:
   // static void SaveScore(const std::string& filename, const ScoreEntry& entry);
    static std::vector<ScoreEntry> LoadScores(const std::string& filename);
    static void SaveScore(const std::string& name, int score);
};
