#pragma once
#include <string>

// �浵�ļ�
extern const std::string slotFiles[3];

// �浵�ӿ�
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
