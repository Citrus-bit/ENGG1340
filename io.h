#pragma once
#include <string>

// 存档文件
extern const std::string slotFiles[3];

// 存档接口
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
