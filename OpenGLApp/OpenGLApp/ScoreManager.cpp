#include "ScoreManager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

/*void ScoreManager::SaveScore(const std::string& filename, const ScoreEntry& entry) {
    std::ofstream file(filename, std::ios::app);
    if (file.is_open()) {
        file << entry.name << "," << entry.score << std::endl;
        file.close();
    }
    else {
        std::cerr << "Errore nel salvataggio del punteggio su file." << std::endl;
    }
}*/


std::vector<ScoreEntry> ScoreManager::LoadScores(const std::string& filename) {
    std::vector<ScoreEntry> scores;
    std::ifstream file(filename);
    std::string line;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name;
        int score;

        if (std::getline(ss, name, ',') && ss >> score) {
            scores.push_back({ name, score });
        }
    }

    // Ordinamento decrescente in base al punteggio
    std::sort(scores.begin(), scores.end(), [](const ScoreEntry& a, const ScoreEntry& b) {
        return b.score < a.score;
        });

    // Limita la classifica ai primi 10 punteggi
    if (scores.size() > 10) {
        scores.resize(10);
    }

    return scores;
}


void ScoreManager::SaveScore(const std::string& name, int score) {
    std::vector<std::pair<std::string, int>> scores;
    std::ifstream inFile("score.txt");

    std::string line;
    while (std::getline(inFile, line)) {
        std::istringstream iss(line);
        std::string player;
        int s;
        if (iss >> player >> s) {
            scores.emplace_back(player, s);
        }
    }
    inFile.close();

    scores.emplace_back(name, score);
    std::sort(scores.begin(), scores.end(), [](auto& a, auto& b) {
        return b.second > a.second;
        });

    std::ofstream file("score.txt", std::ios::app);
    if (file.is_open()) {
        file << name << "," << score << "\n";
        file.close();
    } else {
        std::cerr << "Errore: impossibile aprire score.txt per scrittura.\n";
    }

    file.close();

}

