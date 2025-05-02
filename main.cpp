#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <fstream>
#include <functional>
#include <random>
#include <cctype>
#include "io.h"

using namespace std;

// —— Configuration —— 
const int ROW_W = 4;
const int FRAME_TIME = 250;

// —— Forward Declarations —— 
struct Monster;
struct Tower;
struct Projectile;
struct Item;

void damageUpgrade();
void arcBoostThisWave();
void placeSlowTrap(double seconds);
void roadBomb();
void placeFrostTowerItem();

void clearScreen();
void initPlusCols();
void draw();

void spawnMon();
void moveMon();
void updateProjectiles();

void fire(int tid, Tower& t);
void updateTowers(double dt);

void placeTower();
void waitPlacement();
bool isOccupied(int r, int c);



void usePrepareItems();
void showShop();

// —— Data Structures —— 
struct Monster {
    double row, col;
    int hp;
    bool alive, isElite;
    double slowTimer = 0.0;
};

struct Tower {
    int row, col, damage;
    bool isAOE;
    double rate, cooldown;
    bool isFrost = false;
};

struct Projectile {
    int target, owner;
    double row, col;
    char ch;
    int damage;
    bool alive;
};

enum ItemType { Immediate, Prepare };

struct Item {
    string name;
    int cost;
    ItemType type;
    function<void()> applyImmediate;
    function<void()> applyPrepare;
};

// —— Global Variables —— 
int playerHP, playerGold;
int spawnCd, waveNumber, toSpawn, baseHP;
bool paused;
int difficulty;               // Difficulty
int maxHP;                    // Maximum HP
double goldMultiplier;        // Current wave gold multiplier
double goldMultiplierNextWave;// Next wave gold multiplier
int arcBoostThisWaveCount;    // Additional targets for Arc towers this wave

vector<Tower>     towers;
vector<Monster>   monsters;
vector<Projectile> projectiles;
vector<Item>      allItems;
vector<Item>      inventoryPrepare;

int    damageUpgradeCount = 0;
bool   arcBoostThisWaveFlag = false;
double slowTrapRemaining = 0.0;


void startNewGame(int diff) {
    difficulty = diff;
    maxHP = 10;
    playerHP = maxHP;
    playerGold = 100;
    goldMultiplier = goldMultiplierNextWave = 1.0;
    arcBoostThisWaveCount = 0;
    towers.clear();
    monsters.clear();
    projectiles.clear();
    waveNumber = 0;
}
// Map and Coordinates
vector<string> gameMap = {
    "   +   +   +   +   +   +   +   +   +   +   +   +   +   ",
    "--------------------------------------------------------",
    "S                                                      |",
    "S                                                      |",
    "S                                                      |",
    "S                                                      |",
    "S                                                      |",
    "--------------------------------------------------------",
    "   +   +   +   +   +   +   +   +   +   +   +   +   +   ",
    "--------------------------------------------------------",
    "|                                                      |",
    "|                                                      |",
    "|                                                      |",
    "|                                                      |",
    "|                                                      |",
    "--------------------------------------------------------",
    "   +   +   +   +   +   +   +   +   +   +   +   +   +   ",
    "--------------------------------------------------------",
    "|                                                      H",
    "|                                                      H",
    "|                                                      H",
    "|                                                      H",
    "|                                                      H",
    "--------------------------------------------------------",
    "   +   +   +   +   +   +   +   +   +   +   +   +   +   "
};
vector<int> plusCols;
vector<int> plusRows = { 0,8,16,24 };

// —— Implementation —— 

// Item Effects
void damageUpgrade() {
    ++damageUpgradeCount;
    for (auto& t : towers) t.damage += 1;
}
void arcBoostThisWave() {
    arcBoostThisWaveFlag = true;
}
void placeSlowTrap(double seconds) {
    slowTrapRemaining = seconds;
}
void roadBomb() {
    for (auto& m : monsters) if (m.alive) m.hp = 0;
}
void placeFrostTowerItem() {
    int r; char lc;
    cout << "Select frost tower row (0/8/16/24): "; cin >> r; cin.ignore(numeric_limits<streamsize>::max(), '\n');
    cout << "Select frost tower column (0-9,A-C): "; cin >> lc; cin.ignore(numeric_limits<streamsize>::max(), '\n');
    int idx = isdigit(lc) ? lc - '0' : toupper(lc) - 'A' + 10;
    int col = plusCols[idx];
    if (isOccupied(r, col)) {
        cout << "This position is already occupied!\n";
        return;
    }
    towers.push_back({ r,col,1,true,1.0,0.0,true });
}

// Utilities
void clearScreen() {
    cout << "\x1B[2J\x1B[H";
}
void initPlusCols() {
    plusCols.clear();
    for (int c = 0; c < (int)gameMap[0].size(); ++c)
        if (gameMap[0][c] == '+')
            plusCols.push_back(c);
}
bool isOccupied(int r, int c) {
    for (auto& t : towers)
        if (t.row == r && t.col == c) return true;
    return false;
}

// Rendering
void draw() {
    clearScreen();
    // Top numbering
    cout << string(ROW_W, ' ');
    for (int c = 0;c < (int)gameMap[0].size();++c) {
        if (gameMap[0][c] == '+') {
            int idx = find(plusCols.begin(), plusCols.end(), c) - plusCols.begin();
            cout << char(idx < 10 ? '0' + idx : 'A' + idx - 10);
        }
        else cout << ' ';
    }
    cout << "\n";
    // Make a copy
    auto buf = gameMap;
    // Towers
    for (auto& t : towers)
        buf[t.row][t.col] = t.isFrost ? 'F' : (t.isAOE ? 'E' : 'A');
    // Monsters
    for (auto& m : monsters)
        if (m.alive)
            buf[(int)round(m.row)][(int)round(m.col)] = m.isElite ? 'E' : 'M';
    // Projectiles
    for (auto& p : projectiles) if (p.alive) {
        int pr = (int)round(p.row), pc = (int)round(p.col);
        if (pr >= 0 && pr < (int)buf.size() && pc >= 0 && pc < (int)buf[0].size())
            buf[pr][pc] = p.ch;
    }
    // Output rows
    for (int r = 0;r < (int)buf.size();++r) {
        bool show = find(plusRows.begin(), plusRows.end(), r) != plusRows.end();
        if (show) cout << setw(3) << r << ' ';
        else      cout << "    ";
        cout << buf[r] << "\n";
    }
    cout << "HP:" << playerHP << "/" << maxHP
        << "  Gold:" << playerGold
        << "  Wave:" << waveNumber
        << (paused ? "  [Paused]" : "")
        << "  Diff:" << (difficulty == 1 ? "Easy" : difficulty == 2 ? "Normal" : "Hard")
        << "\n";
}

// Monster Spawning and Movement
void spawnMon() {
    if (toSpawn > 0 && spawnCd == 0) {
        int rs[3] = { 3,4,5 };
        int r = rs[rand() % 3];
        bool e = (rand() % 5) == 0;
        monsters.push_back({ (double)r,0.0,baseHP * (e ? 2 : 1),true,e });
        --toSpawn; spawnCd = 6;
    }
    else if (spawnCd > 0) --spawnCd;
}
void moveMon() {
    for (auto& m : monsters) if (m.alive) {
        double speed = 1.0;
        if (m.slowTimer > 0) {
            speed *= 0.5;
            m.slowTimer = max(0.0, m.slowTimer - FRAME_TIME / 1000.0);
        }
        m.col += speed;
        if ((int)m.col >= (int)gameMap[(int)m.row].size() - 1) {
            if (m.row < 16) { m.row += 8; m.col = 0; }
            else { --playerHP; m.alive = false; }
        }
    }
}

// Tower Firing
void fire(int tid, Tower& t) {
    int idx = find(plusRows.begin(), plusRows.end(), t.row) - plusRows.begin();
    vector<int> segs;
    if (idx > 0) segs.push_back(idx - 1);
    if (idx < (int)plusRows.size() - 1) segs.push_back(idx);
    if (!t.isAOE) {
        for (auto& p : projectiles)
            if (p.owner == tid && p.alive) return;
    }
    vector<pair<double, int>> cand;
    for (int i = 0;i < (int)monsters.size();++i) {
        auto& m = monsters[i];
        if (!m.alive) continue;
        int seg = -1;
        for (int j = 0;j + 1 < (int)plusRows.size();++j)
            if (m.row > plusRows[j] && m.row < plusRows[j + 1]) { seg = j; break; }
        if (find(segs.begin(), segs.end(), seg) == segs.end()) continue;
        double d = fabs(m.row - t.row) + fabs(m.col - t.col);
        cand.emplace_back(d, i);
    }
    if (cand.empty()) return;
    sort(cand.begin(), cand.end());
    char ch = t.isFrost ? '*' : (t.isAOE ? '~' : '.');
    int baseShots = t.isAOE ? 2 : 1;
    int shots = min((int)cand.size(), baseShots + arcBoostThisWaveCount);

    for (int k = 0;k < shots;++k) {
        int mi = cand[k].second;
        projectiles.push_back({ mi,tid,(double)t.row,(double)t.col,ch,t.damage,true });
    }
    t.cooldown = 1.0 / t.rate;
}
void updateTowers(double dt) {
    for (int i = 0;i < (int)towers.size();++i) {
        towers[i].cooldown -= dt;
        if (towers[i].cooldown <= 0) fire(i, towers[i]);
    }
}

// Projectile Updates
void updateProjectiles() {
    const double speed = 2.0;
    for (auto& p : projectiles) if (p.alive) {
        if (p.target < 0 || p.target >= (int)monsters.size() || !monsters[p.target].alive) {
            p.alive = false; continue;
        }
        auto& m = monsters[p.target];
        double dx = m.row - p.row, dy = m.col - p.col;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist <= speed) { p.row = m.row; p.col = m.col; }
        else {
            p.row += dx / dist * speed;
            p.col += dy / dist * speed;
        }
        int pr = (int)round(p.row), pc = (int)round(p.col);
        if (pr == (int)round(m.row) && pc == (int)round(m.col)) {
            m.hp -= p.damage;
            if (m.hp <= 0) {
                m.alive = false;
                int drop = m.isElite ? 10 : 5;
                playerGold += int(drop * goldMultiplier);
            }

            if (p.ch == '*') m.slowTimer = 2.0;
            p.alive = false;
        }
    }
    projectiles.erase(
        remove_if(projectiles.begin(), projectiles.end(), [](auto& p) {return !p.alive;}),
        projectiles.end()
    );
}

// Placement Phase
void placeTower() {
    draw();
    char ch;
    // —— Modified here —— 
    do {
        cout << "Place Tower: (1) Arrow (20)  2) Arc (30)\n";
        cin >> ch;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (ch != '1' && ch != '2') {
            cout << "Invalid selection, please enter 1 or 2.\n";
        }
    } while (ch != '1' && ch != '2');
    int cost = (ch == '2') ? 30 : 20;
    // —— Check if enough gold, return if not —— 
    if (playerGold < cost) {
        cout << "Not enough gold!\n";
        this_thread::sleep_for(chrono::milliseconds(800));
        return;
    }
    int r;
    do {
        cout << "Row? (0/8/16/24) ";
        cin >> r; cin.ignore(numeric_limits<streamsize>::max(), '\n');
    } while (find(plusRows.begin(), plusRows.end(), r) == plusRows.end());

    char lc; int idx, col;
    do {
        cout << "Col? (0-9,A-C) ";
        cin >> lc; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        idx = isdigit(lc) ? lc - '0' : toupper(lc) - 'A' + 10;
    } while (idx < 0 || idx >= (int)plusCols.size());
    col = plusCols[idx];

    if (isOccupied(r, col)) {
        cout << "This position is already occupied!\n";
        this_thread::sleep_for(chrono::milliseconds(800));
        return;
    }
    towers.push_back({ r,col,(ch == '2' ? 1 : 2),(ch == '2'),(ch == '2' ? 1.0 : 2.0),0.0,false });
    playerGold -= cost;
}

// Wait for Placement
void waitPlacement() {
    draw();
    cout << "Placement Phase:\n  [Enter] Start Wave  [b] Place Tower  [i] Inventory\n";
    string s;
    while (true) {
        getline(cin, s);
        if (s.empty()) break;
        char c = tolower(s[0]);
        if (c == 'b') placeTower();
        else if (c == 'i') usePrepareItems();
        draw();
        cout << "Placement Phase:\n  [Enter] Start Wave  [b] Place Tower  [i] Inventory\n";
    }
}



// Inventory and Shop
void usePrepareItems() {
    if (inventoryPrepare.empty()) {
        cout << "Inventory is empty!\n";
        return;
    }
    while (true) {
        cout << "\n=== Inventory ===\n";
        for (int i = 0; i < (int)inventoryPrepare.size(); ++i)
            cout << i + 1 << ") " << inventoryPrepare[i].name << "\n";
        cout << "0) Return\nSelect: ";
        int sel;
        cin >> sel;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (sel == 0) break;
        if (sel < 1 || sel >(int)inventoryPrepare.size()) continue;

        // Record current tower count
        size_t beforeTowers = towers.size();

        // Try to use the item
        inventoryPrepare[sel - 1].applyPrepare();

        // If it's "Frost Fortress", check if tower placement was successful
        if (inventoryPrepare[sel - 1].name == "Frost Fortress (Place frost tower)") {
            if (towers.size() > beforeTowers) {
                // Placement successful: remove from inventory
                inventoryPrepare.erase(inventoryPrepare.begin() + (sel - 1));
            }
            else {
                // Placement failed: position occupied, keep in inventory
                cout << "Placement failed, position already occupied, returned to inventory.\n";
                this_thread::sleep_for(chrono::milliseconds(800));
            }
        }
        else {
            // Other prepare-type items are consumed directly
            inventoryPrepare.erase(inventoryPrepare.begin() + (sel - 1));
        }
        break;
    }
}


void showShop() {
    static mt19937 rng((unsigned)time(nullptr));
    if (allItems.size() < 3) {
        // Initialize item list once
        allItems = {
            // **Healing Item**
            { "Healing Potion (Restore 10HP)",  50, Immediate,
                []() {
                    playerHP = min(playerHP + 10, maxHP);
                }, nullptr
            },

            // **Increase Max HP**
            { "Sturdy Amulet (+5 Max HP)",  80, Immediate,
                []() {
                    maxHP += 5;
                    playerHP = maxHP;
                }, nullptr
            },

            // **Arc Tower +1 Target This Wave**
            { "Rift Arc+ (+1 Target)", 75, Immediate,
                []() {
                // Immediately give Arc towers +1 target this wave
                arcBoostThisWaveCount += 1;
            },
            nullptr
    },

            // **All Towers Permanent +1 Damage**
            { "Destruction Amplifier (+1 All Tower Damage)",100, Immediate,
                []() {
                    for (auto& t : towers) t.damage += 1;
                    // Future towers also get +1, can add damageUpgradeCount variable for new towers
                }, nullptr
            },

            // **Next Wave Gold Income ×2**
            { "Wealth Badge+ (Next Wave Gold ×2)",120, Prepare,
                nullptr,
                []() {

                    goldMultiplierNextWave = 2.0;
                }
            },

            // **Place Frost Tower**
            { "Frost Fortress (Place frost tower)",150, Prepare,
                nullptr,
                placeFrostTowerItem
            }
        };
    }
    shuffle(allItems.begin(), allItems.end(), rng);
    vector<Item> offer(allItems.begin(), allItems.begin() + 3);

    while (true) {
        clearScreen();
        cout << "\n--- Shop ---\nCurrent Gold: " << playerGold << "\n";
        // Only show remaining items in current offer
        for (int i = 0; i < (int)offer.size(); ++i) {
            cout << (i + 1) << ") " << offer[i].name
                << " Price:" << offer[i].cost << "\n";
        }
        cout << "0) Exit Shop\nSelect: ";

        int sel;
        cin >> sel;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        if (sel == 0) {
            clearScreen();
            break;
        }
        if (sel < 1 || sel >(int)offer.size()) {
            cout << "Invalid selection\n";
            this_thread::sleep_for(chrono::milliseconds(500));
            continue;
        }

        // Player selected offer[sel-1]
        Item it = offer[sel - 1];
        if (playerGold < it.cost) {
            cout << "Not enough gold!\n";
            this_thread::sleep_for(chrono::milliseconds(500));
            continue;
        }

        // Deduct gold and apply item
        playerGold -= it.cost;
        if (it.type == Immediate) {
            it.applyImmediate();
        }
        else {
            inventoryPrepare.push_back(it);
        }

        // **Key step**: Remove purchased item from current offer
        offer.erase(offer.begin() + (sel - 1));

        cout << "Purchased: " << it.name << "\n";
        this_thread::sleep_for(chrono::milliseconds(500));

        // If offer is empty, exit shop
        if (offer.empty()) {
            clearScreen();
            break;
        }
    }
}

// Main Loop
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    srand((unsigned)time(nullptr));

    initPlusCols();

    while (true) {
        clearScreen();
        cout << "=== Tower Defense ===\n"
            << "1) New Game\n"
            << "2) Load Slot 1" << (ifstream(slotFiles[0]) ? "\n" : " (empty)\n")
            << "3) Load Slot 2" << (ifstream(slotFiles[1]) ? "\n" : " (empty)\n")
            << "4) Load Slot 3" << (ifstream(slotFiles[2]) ? "\n" : " (empty)\n")
            << "5) Quit\n"
            << "Select: ";
        int choice; cin >> choice; cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if (choice == 5) break;

        if (choice >= 2 && choice <= 4) {
            if (!loadGame(choice - 2,
                difficulty,
                playerHP,
                playerGold,
                waveNumber))
            {
                cout << "Load failed, press Enter to continue";
                cin.get();
                continue;
            }
        }

        else if (choice == 1) {
            cout << "Select Difficulty (1-Easy 2-Normal 3-Hard): ";
            cin >> difficulty; cin.ignore(numeric_limits<streamsize>::max(), '\n');
            startNewGame(difficulty);
        }
        else continue;

        clearScreen();
        cout << "Press Enter to enter placement phase..."; cin.get();
        waitPlacement();

        auto last = chrono::steady_clock::now();
        bool running = true;

        while (running && playerHP > 0) {
            arcBoostThisWaveFlag = false;
            arcBoostThisWaveCount = 0;
            goldMultiplier = goldMultiplierNextWave;
            goldMultiplierNextWave = 1.0;
            ++waveNumber;
            draw();

            baseHP = 5 + (waveNumber - 1) * 2;
            toSpawn = waveNumber * (difficulty == 1 ? 3 : (difficulty == 2 ? 5 : 7));
            spawnCd = 0; monsters.clear(); projectiles.clear();

            while (running && playerHP > 0 &&
                (toSpawn > 0 || any_of(monsters.begin(), monsters.end(), [](auto& m) {return m.alive;})))
            {
                if (cin.rdbuf()->in_avail() > 0) {
                    char cmd = cin.get();
                    if (cmd == 'b' || cmd == 'B') placeTower();
                    else if (cmd == 'p' || cmd == 'P') paused = !paused;
                }
                if (paused) {
                    draw();
                    this_thread::sleep_for(chrono::milliseconds(100));
                    continue;
                }

                auto now = chrono::steady_clock::now();
                double dt = chrono::duration<double>(now - last).count();
                last = now;

                spawnMon();
                moveMon();
                updateTowers(dt);
                updateProjectiles();
                draw();
                this_thread::sleep_for(chrono::milliseconds(FRAME_TIME));

                monsters.erase(
                    remove_if(monsters.begin(), monsters.end(), [](auto& m) {return !m.alive;}),
                    monsters.end()
                );
            }

            if (playerHP <= 0) break;

            // 检查是否达到最大波次
            int maxWaves = (difficulty == 1) ? 3 : (difficulty == 2) ? 5 : 7;
            if (waveNumber >= maxWaves) {
                draw();
                cout << "===== VICTORY! =====\n";
                cout << "Congratulations! You've successfully defended against all " << maxWaves << " waves!\n";
                cout << "Final Stats:\n";
                cout << "- Difficulty: " << (difficulty == 1 ? "Easy" : difficulty == 2 ? "Normal" : "Hard") << "\n";
                cout << "- HP Remaining: " << playerHP << "/" << maxHP << "\n";
                cout << "- Gold Remaining: " << playerGold << "\n";
                cout << "- Towers Built: " << towers.size() << "\n";
                cout << "\nPress Enter to return to main menu...";
                cin.get();
                break;  // 退出游戏循环
            }

            char post;
            do {
                draw();
                cout << "Wave " << waveNumber << " completed! S)Save N)Next Wave Q)Quit\nSelect: ";
                cin >> post; cin.ignore(numeric_limits<streamsize>::max(), '\n');
                if (post == 'S' || post == 's') {
                    cout << "Save slot (1-3): ";
                    int sl; cin >> sl; cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    if (sl >= 1 && sl <= 3) {
                        saveGame(sl - 1,
                            difficulty,
                            playerHP,
                            playerGold,
                            waveNumber);
                        cout << "Saved\n";
                    }
                }

            } while (post == 'S' || post == 's');

            if (post == 'Q' || post == 'q') running = false;
            if (running) {
                showShop();
                cout << "Shop closed, press Enter to continue placement";cin.get();
                waitPlacement();
            }
        }

        if (playerHP <= 0) { clearScreen(); cout << "Game Over! Press Enter to exit"; cin.get(); }
    }

    return 0;
}