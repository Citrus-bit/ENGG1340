#pragma once
#include <string>

// Archive Files
extern const std::string slotFiles[3];

// Archive interface
void saveGame(int slot,
    int difficulty,
    int playerHP,
    int playerGold,
    int waveNumber);
bool loadGame(int slot,
    int& difficulty,
    int& playerHP,
    int& playerGold,
    int& waveNumber);
