#include "io.h"
#include <fstream>

const std::string slotFiles[3] = {
    "slot1.dat", "slot2.dat", "slot3.dat"
};

void saveGame(int slot,
    int difficulty,
    int playerHP,
    int playerGold,
    int waveNumber)
{
    std::ofstream f(slotFiles[slot], std::ios::binary);
    f.write(reinterpret_cast<char*>(&difficulty), sizeof(difficulty));
    f.write(reinterpret_cast<char*>(&playerHP), sizeof(playerHP));
    f.write(reinterpret_cast<char*>(&playerGold), sizeof(playerGold));
    f.write(reinterpret_cast<char*>(&waveNumber), sizeof(waveNumber));
}

bool loadGame(int slot,
    int& difficulty,
    int& playerHP,
    int& playerGold,
    int& waveNumber)
{
    std::ifstream f(slotFiles[slot], std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(&difficulty), sizeof(difficulty));
    f.read(reinterpret_cast<char*>(&playerHP), sizeof(playerHP));
    f.read(reinterpret_cast<char*>(&playerGold), sizeof(playerGold));
    f.read(reinterpret_cast<char*>(&waveNumber), sizeof(waveNumber));
    return true;
}
