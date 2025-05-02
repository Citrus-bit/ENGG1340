#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <ctime>
#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
bool _kbhit() {
    static bool initialized = false;
    if (!initialized) {
        termios term;
        tcgetattr(0, &term);
        term.c_lflag &= ~ICANON;
        tcsetattr(0, TCSANOW, &term);
        setbuf(stdin, nullptr);
        initialized = true;
    }
    int bytesWaiting;
    ioctl(0, FIONREAD, &bytesWaiting);
    return bytesWaiting > 0;
}
int _getch() { return getchar(); }
#endif
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <sstream>

using namespace std;

// Add flushStdin function declaration
void flushStdin() {
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

// --- Map ---
vector<string> gameMap;
vector<int> plusCols;
vector<int> plusRows;

struct Monster {
    double row, col;
    int hp;
    bool alive, isElite;
    double slowEffect;  // 减速效果，1.0表示正常速度，小于1.0表示减速
    double slowDuration;  // 减速持续时间
};
struct Tower {
    int row, col, damage;
    bool isAOE;
    bool isFreeze;  // 是否为冰冻塔
    double rate, cooldown;
    double slowFactor;  // 减速因子
};
struct Projectile {
    vector<pair<int, int>> path;
    size_t idx;
    int owner;
    char ch;
    int damage;
    bool isTracking;      // 是否为追踪子弹
    bool isAOE;           // 是否为AOE攻击
    Monster* target;      // 追踪的目标
    double speed;         // 移动速度
    double x, y;          // 当前精确位置
};

vector<Monster> monsters;
vector<Tower> towers;
vector<Projectile> projectiles;

// 在文件开头保留一组变量声明
int playerHP = 10;
int playerGold = 100;
int spawnCd = 0;
int waveNumber = 0;
int toSpawn = 0;
int baseHP = 5;
int maxWaves = 5;  // 默认为普通模式的5波

bool paused = false;

const int ROW_W = 4;
const int FRAME_TIME = 100;

// 在难度相关变量区域
enum Difficulty { EASY, NORMAL, HARD };
Difficulty gameDifficulty = NORMAL;
double difficultyMultiplier = 1.0;
int spawnRateModifier = 6;
int initialGold = 100;
int initialHP = 10;

// --- Tools ---
void clearScreen() {
#ifdef _WIN32
    system("cls");
#else
    cout << "\033[2J\033[1;1H";
#endif
}

void initPlusCols() {
    plusCols.clear();
    for (int c = 0; c < (int)gameMap[0].size(); ++c) {
        if (gameMap[0][c] == '+') {
            plusCols.push_back(c);
        }
    }
}

vector<pair<int, int> > bres(int r0, int c0, int r1, int c1) {
    vector<pair<int, int> > v;
    int dr = abs(r1 - r0), dc = abs(c1 - c0);
    int sr = (r1 > r0) ? 1 : -1, sc = (c1 > c0) ? 1 : -1;
    int err = dr - dc;
    while (true) {
        v.push_back({ r0, c0 });
        if (r0 == r1 && c0 == c1) break;
        int e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r0 += sr; }
        if (e2 < dr) { err += dr; c0 += sc; }
    }
    return v;
}

// --- Rendering ---
void drawBuffer();
void updateGame(double dt);

void drawBuffer() {
    static string lastFrame = "";
    static stringstream ss;
    ss.str("");
    
    // 构建当前帧
    ss << string(ROW_W, ' ');
    for (int c = 0; c < (int)gameMap[0].size(); ++c) {
        if (gameMap[0][c] == '+') {
            int idx = find(plusCols.begin(), plusCols.end(), c) - plusCols.begin();
            char lb = (idx < 10 ? '0' + idx : 'A' + (idx - 10));
            ss << lb;
        }
        else ss << ' ';
    }
    ss << "\n";

    auto buf = gameMap;
    
    // 标记塔和不可建造位置
    for (int r : plusRows) {
        for (size_t i = 0; i < plusCols.size(); ++i) {
            int c = plusCols[i];
            
            // 检查是否已有塔
            bool hasTower = false;
            for (const auto& t : towers) {
                if (t.row == r && t.col == c) {
                    hasTower = true;
                    break;
                }
            }
            
            // 如果没有塔，检查相邻位置是否有塔
            if (!hasTower) {
                bool adjacentHasTower = false;
                for (int j = -1; j <= 1; j += 2) {
                    int adjIdx = i + j;
                    if (adjIdx >= 0 && adjIdx < (int)plusCols.size()) {
                        int adjacentCol = plusCols[adjIdx];
                        for (const auto& t : towers) {
                            if (t.row == r && t.col == adjacentCol) {
                                adjacentHasTower = true;
                                break;
                            }
                        }
                        if (adjacentHasTower) break;
                    }
                }
                
                // 如果相邻位置有塔，标记为不可建造
                if (adjacentHasTower) {
                    buf[r][c] = '-';
                }
            }
        }
    }
    
    // 绘制塔、怪物和子弹
    for (auto& t : towers) {
        if (t.isFreeze) buf[t.row][t.col] = 'F';
        else buf[t.row][t.col] = t.isAOE ? 'B' : 'A';
    }
    for (auto& m : monsters) if (m.alive) {
        int r = (int)round(m.row), c = (int)round(m.col);
        buf[r][c] = m.isElite ? 'E' : 'M';
    }
    for (auto& p : projectiles) {
        if (p.isTracking) {
            // 追踪子弹显示
            int r = (int)round(p.x);
            int c = (int)round(p.y);
            if (r >= 0 && r < (int)buf.size() && c >= 0 && c < (int)buf[r].size())
                buf[r][c] = p.ch;
        } else if (p.idx < p.path.size()) {
            // 非追踪子弹显示
            auto [r, c] = p.path[p.idx];
            if (r >= 0 && r < (int)buf.size() && c >= 0 && c < (int)buf[r].size())
                buf[r][c] = p.ch;
        }
    }

    for (int r = 0; r < (int)buf.size(); ++r) {
        bool show = find(plusRows.begin(), plusRows.end(), r) != plusRows.end();
        if (show) ss << setw(ROW_W - 1) << r << ' ';
        else      ss << string(ROW_W, ' ');
        ss << buf[r] << "\n";
    }
    ss << "HP:" << playerHP << "  Gold:" << playerGold << "  Wave:" << waveNumber << "/" << maxWaves;
    if (paused) ss << "  [Paused]";
    ss << "\n";

    string currentFrame = ss.str();
    
    // 只有当画面变化时才重绘
    if (currentFrame != lastFrame) {
        clearScreen();
        cout << currentFrame;
        lastFrame = currentFrame;
    }
}

// --- Core Logic ---
void initGame() {
    // 初始化地图 - 增加宽度
    gameMap.push_back("   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   ");
    gameMap.push_back("--------------------------------------------------------------------------------");
    gameMap.push_back("S                                                                              |");
    gameMap.push_back("S                                                                              |");
    gameMap.push_back("S                                                                              |");
    gameMap.push_back("S                                                                              |");
    gameMap.push_back("S                                                                              |");
    gameMap.push_back("--------------------------------------------------------------------------------");
    gameMap.push_back("   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   ");
    gameMap.push_back("--------------------------------------------------------------------------------");
    gameMap.push_back("|                                                                              |");
    gameMap.push_back("|                                                                              |");
    gameMap.push_back("|                                                                              |");
    gameMap.push_back("|                                                                              |");
    gameMap.push_back("|                                                                              |");
    gameMap.push_back("--------------------------------------------------------------------------------");
    gameMap.push_back("   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   ");
    gameMap.push_back("--------------------------------------------------------------------------------");
    gameMap.push_back("|                                                                              H");
    gameMap.push_back("|                                                                              H");
    gameMap.push_back("|                                                                              H");
    gameMap.push_back("|                                                                              H");
    gameMap.push_back("|                                                                              H");
    gameMap.push_back("--------------------------------------------------------------------------------");
    gameMap.push_back("   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   +   ");

    // 初始化 plusRows
    plusRows.push_back(0);
    plusRows.push_back(8);
    plusRows.push_back(16);
    plusRows.push_back(24);

    // 初始化怪物的减速状态
    for (auto& m : monsters) {
        m.slowEffect = 1.0;
        m.slowDuration = 0.0;
    }
}

// 添加设置难度的函数
void setDifficulty(Difficulty diff) {
    gameDifficulty = diff;
    
    switch (diff) {
        case EASY:
            difficultyMultiplier = 0.75;
            spawnRateModifier = 8;
            initialGold = 150;
            initialHP = 15;
            maxWaves = 2;  // 简单模式只有2波
            break;
        case NORMAL:
            difficultyMultiplier = 1;
            spawnRateModifier = 6;
            initialGold = 100;
            initialHP = 10;
            maxWaves = 5;  // 普通模式有5波
            break;
        case HARD:
            difficultyMultiplier = 1.25;  // 从1.5降低到1.25
            spawnRateModifier = 5;        // 从4增加到5，减慢生成速度
            initialGold = 100;            // 从80增加到100
            initialHP = 8;
            maxWaves = 10;  // 困难模式有10波
            break;
    }
    
    // 重置游戏状态
    playerGold = initialGold;
    playerHP = initialHP;
}

// 修改spawnMon函数以使用难度设置
void spawnMon() {
    if (toSpawn > 0 && spawnCd == 0) {
        int rs[3] = { 3, 4, 5 };
        int r = rs[rand() % 3];
        bool e = rand() % 5 == 0;
        Monster m;
        m.row = double(r);
        m.col = 0;
        m.hp = int(baseHP * (e ? 2 : 1) * difficultyMultiplier);
        m.alive = true;
        m.isElite = e;
        m.slowEffect = 1.0;  // 初始无减速
        m.slowDuration = 0.0;
        monsters.push_back(m);
        --toSpawn; spawnCd = spawnRateModifier * 2;
    }
    else if (spawnCd > 0) --spawnCd;
}

void moveMon() {
    for (auto& m : monsters) if (m.alive) {
        // 更新减速持续时间
        if (m.slowDuration > 0) {
            m.slowDuration -= 0.1;  // 假设每次更新减少0.1秒
            if (m.slowDuration <= 0) {
                m.slowEffect = 1.0;  // 恢复正常速度
            }
        }
        
        if ((int)m.col >= (int)gameMap[m.row].size() - 1) {
            if (m.row < 16) { m.row += 8; m.col = 0; }
            else { playerHP--; m.alive = false; }
        }
        else {
            // 应用减速效果
            m.col += 0.5 * m.slowEffect;
        }
    }
}

void fire(int tid, Tower& t) {
    if (any_of(projectiles.begin(), projectiles.end(), [&](auto& p) { return p.owner == tid && p.idx < p.path.size(); }))
        return;
    
    Monster* target = nullptr;
    double bestD = 1e9;
    for (auto& m : monsters) if (m.alive) {
        double d = abs(m.row - t.row) + abs(m.col - t.col);
        if (d < bestD) { bestD = d; target = &m; }
    }
    if (!target) return;
    
    Projectile p;
    if (t.isFreeze) {  // Freeze 塔使用追踪子弹
        p.isTracking = true;
        p.target = target;
        p.x = t.row;
        p.y = t.col;
        p.speed = 2.0;  // 冰冻子弹速度
        p.path = bres(t.row, t.col, (int)round(target->row), (int)round(target->col));
    } else if (!t.isAOE) {  // Arrow 塔使用追踪子弹
        p.isTracking = true;
        p.target = target;
        p.x = t.row;
        p.y = t.col;
        p.speed = 1.5;  // 降低追踪子弹速度，从3.0降到1.5
        // 仍然生成初始路径用于显示
        p.path = bres(t.row, t.col, (int)round(target->row), (int)round(target->col));
    } else {  // Arc 塔使用原来的路径子弹
        p.isTracking = false;
        p.isAOE = true;  // 设置为AOE攻击
        p.path = bres(t.row, t.col, (int)round(target->row), (int)round(target->col));
    }
    
    p.idx = 0;
    p.owner = tid;
    p.ch = t.isFreeze ? '@' : (t.isAOE ? '~' : '*');
    p.damage = t.damage;
    projectiles.push_back(p);
    t.cooldown = 1.0 / t.rate;
}

void updateTowers(double dt) {
    for (size_t i = 0; i < towers.size(); ++i) {
        Tower& t = towers[i];
        t.cooldown -= dt;
        if (t.cooldown <= 0) fire((int)i, t);
    }
}

void updateProjectiles() {
    for (auto& p : projectiles) {
        if (p.isTracking) {
            // 检查目标是否有效
            if (!p.target || !p.target->alive) {
                // 目标无效，尝试寻找新目标
                p.target = nullptr;
                double bestD = 20.0; // 设置最大搜索距离
                for (auto& m : monsters) {
                    if (m.alive) {
                        double d = sqrt(pow(m.row - p.x, 2) + pow(m.col - p.y, 2));
                        if (d < bestD) {
                            bestD = d;
                            p.target = &m;
                        }
                    }
                }
                
                // 如果找不到新目标，标记为已完成
                if (!p.target) {
                    p.idx = p.path.size();
                    continue;
                }
            }
            
            // 计算方向向量
            double dx = p.target->row - p.x;
            double dy = p.target->col - p.y;
            double len = sqrt(dx*dx + dy*dy);
            
            // 添加最大追踪时间检查
            static const int MAX_TRACKING_STEPS = 100;
            if (p.path.size() > MAX_TRACKING_STEPS) {
                // 如果追踪时间过长，直接命中目标
                p.target->hp -= p.damage;
                
                // 检查是否是冰冻塔的子弹
                for (const auto& t : towers) {
                    if (t.isFreeze && p.owner == &t - &towers[0]) {
                        // 应用减速效果
                        p.target->slowEffect = t.slowFactor;
                        p.target->slowDuration = 3.0;  // 减速持续3秒
                        break;
                    }
                }
                
                if (p.target->hp <= 0) {
                    p.target->alive = false;
                    playerGold += p.target->isElite ? 20 : 10;
                }
                
                // 标记为已完成
                p.idx = p.path.size();
                p.target = nullptr;
                continue;
            }
            
            // 更强的接近检测 - 如果子弹已经非常接近目标，直接命中
            if (len < 0.8) {  // 增加命中范围从0.5到0.8
                // 造成伤害
                p.target->hp -= p.damage;
                
                // 检查是否是冰冻塔的子弹
                for (const auto& t : towers) {
                    if (t.isFreeze && p.owner == &t - &towers[0]) {
                        // 应用减速效果
                        p.target->slowEffect = t.slowFactor;
                        p.target->slowDuration = 3.0;  // 减速持续3秒
                        break;
                    }
                }
                
                if (p.target->hp <= 0) {
                    p.target->alive = false;
                    playerGold += p.target->isElite ? 20 : 10;
                }
                
                // 标记为已完成，确保子弹消失
                p.idx = p.path.size();
                p.target = nullptr;
            } else {
                // 检测是否在原地打转
                bool stuckInPlace = false;
                if (p.path.size() >= 10) {
                    // 计算最近10步的移动距离
                    double totalMovement = 0;
                    for (size_t i = p.path.size() - 10; i < p.path.size() - 1; i++) {
                        totalMovement += sqrt(
                            pow(p.path[i+1].first - p.path[i].first, 2) + 
                            pow(p.path[i+1].second - p.path[i].second, 2)
                        );
                    }
                    
                    // 如果总移动距离很小，认为子弹在原地打转
                    if (totalMovement < 3.0 && len > 1.0) {
                        stuckInPlace = true;
                    }
                }
                
                // 如果子弹在原地打转或路径过长，使用更直接的追踪方式
                if (stuckInPlace || p.path.size() > 30) {
                    // 直接向目标移动，不考虑其他因素
                    dx /= len;
                    dy /= len;
                    
                    // 增加速度，使子弹更快地接近目标
                    double speedMultiplier = 1.5;
                    p.x += dx * p.speed * speedMultiplier;
                    p.y += dy * p.speed * speedMultiplier;
                } else {
                    // 正常追踪逻辑
                    dx /= len;
                    dy /= len;
                    
                    // 添加小随机偏移，防止卡在某个位置
                    if (p.path.size() > 10 && len > 1.0) {
                        // 添加随机偏移，但幅度更小
                        dx += (rand() % 100 - 50) / 1000.0;
                        dy += (rand() % 100 - 50) / 1000.0;
                        // 重新归一化
                        double newLen = sqrt(dx*dx + dy*dy);
                        dx /= newLen;
                        dy /= newLen;
                    }
                    
                    p.x += dx * p.speed;
                    p.y += dy * p.speed;
                }
                
                // 更新路径显示
                int newRow = (int)round(p.x);
                int newCol = (int)round(p.y);
                
                // 检查是否需要更新路径
                if (p.path.empty() || p.path.back().first != newRow || p.path.back().second != newCol) {
                    if (p.path.size() > 10) {  // 限制路径长度，防止过长
                        p.path.erase(p.path.begin());
                    }
                    p.path.push_back({newRow, newCol});
                }
                
                p.idx = p.path.size() - 1;  // 显示路径的最后一个点
            }
        } else {
            // 非追踪子弹逻辑
            if (p.idx + 1 < p.path.size()) {
                p.idx++;
                auto [r, c] = p.path[p.idx];
                
                // 增加 Arc 塔子弹的攻击判定范围
                bool hit = false;
                for (auto& m : monsters) {
                    if (m.alive) {
                        // 计算怪物与子弹当前位置的距离
                        double dist = sqrt(pow(m.row - r, 2) + pow(m.col - c, 2));
                        
                        // 增大判定范围，从精确命中改为1.5单位范围内命中
                        if (dist <= 1.5) {
                            m.hp -= p.damage;
                            if (m.hp <= 0) {
                                m.alive = false;
                                playerGold += m.isElite ? 20 : 10;
                            }
                            hit = true;
                            
                            // 如果是AOE攻击，继续检查其他怪物；否则跳出循环
                            if (!p.isAOE) {
                                p.idx = p.path.size();
                                break;
                            }
                        }
                    }
                }
                
                // 如果命中了怪物且不是AOE攻击，标记子弹为已完成
                if (hit && !p.isAOE) {
                    p.idx = p.path.size();
                }
            } else {
                p.idx = p.path.size();
            }
        }
    }
    
    // 使用更高效的方式删除已完成的子弹
    vector<Projectile> newProjectiles;
    newProjectiles.reserve(projectiles.size());
    
    for (const auto& p : projectiles) {
        // 修改条件，确保追踪子弹在击中目标后被删除
        if (p.idx < p.path.size() || (p.isTracking && p.target && p.target->alive && p.idx < p.path.size())) {
            newProjectiles.push_back(p);
        }
    }
    
    projectiles.swap(newProjectiles);
}

// --- Input ---
void placeTower() {
    clearScreen(); drawBuffer();
    cout << "\n选择塔类型:\n";
    cout << "1) Arrow塔 (20金币) - 低伤害，高攻速，子弹会追踪目标\n";
    cout << "2) Arc塔 (30金币) - 高伤害，低攻速，子弹沿直线飞行\n";
    cout << "3) Freeze塔 (40金币) - 中等伤害，中等攻速，减缓敌人移动速度\n\n";
    cout << "请选择 (1/2/3): ";
    
    char ch;
    do {
        cin >> ch;
        if (ch != '1' && ch != '2' && ch != '3') {
            cout << "无效输入，请输入1、2或3。\n";
            cin.clear();
            flushStdin();
        }
    } while (ch != '1' && ch != '2' && ch != '3');
    
    int cost;
    if (ch == '1') cost = 20;
    else if (ch == '2') cost = 30;
    else cost = 40;  // Freeze 塔
    
    if (playerGold < cost) {
        cout << "金币不足！\n"; this_thread::sleep_for(chrono::milliseconds(800));
        return;
    }
    
    int r;
    do {
        cout << "\nRow (0/8/16/24): ";
        cin >> r;
        if (cin.fail() || find(plusRows.begin(), plusRows.end(), r) == plusRows.end()) {
            cout << "Invalid row! Try again.\n";
            cin.clear();  // 清除错误状态
            flushStdin(); // 清空输入缓冲区
        }
    } while (cin.fail() || find(plusRows.begin(), plusRows.end(), r) == plusRows.end());
    
    char lc;
    int idx;
    do {
        cout << "选择列位置 (数字0-9或字母A-" << char('A' + plusCols.size() - 11) << "): ";
        cin >> lc;
        cin.clear();  // 清除可能的错误状态
        flushStdin(); // 清空输入缓冲区，防止多余输入影响后续操作
        
        // 转换列标签为索引
        if (isdigit(lc)) {
            idx = lc - '0';
        } else if (isalpha(lc)) {
            idx = toupper(lc) - 'A' + 10;
        } else {
            idx = -1;  // 无效输入
        }
        
        if (idx < 0 || idx >= (int)plusCols.size()) {
            cout << "Invalid column! Try again.\n";
        }
    } while (idx < 0 || idx >= (int)plusCols.size());
    
    // 检查该位置是否已有塔，以及相邻位置是否有塔
    int col = plusCols[idx];
    bool towerExists = false;
    bool adjacentTowerExists = false;
    
    for (const auto& t : towers) {
        // 检查当前位置
        if (t.row == r && t.col == col) {
            towerExists = true;
            break;
        }
        
        // 检查相邻位置（左右）
        for (int i = -1; i <= 1; i += 2) {
            int adjacentIdx = idx + i;
            if (adjacentIdx >= 0 && adjacentIdx < (int)plusCols.size()) {
                int adjacentCol = plusCols[adjacentIdx];
                if (t.row == r && t.col == adjacentCol) {
                    adjacentTowerExists = true;
                    break;
                }
            }
        }
        
        if (towerExists || adjacentTowerExists) break;
    }
    
    if (towerExists) {
        cout << "该位置已有塔！请选择其他位置。\n";
        this_thread::sleep_for(chrono::milliseconds(1000));
        return;
    }
    
    if (adjacentTowerExists) {
        cout << "相邻位置已有塔！必须间隔一个空位。\n";
        this_thread::sleep_for(chrono::milliseconds(1000));
        return;
    }
    
    // 创建塔
    Tower t;
    t.row = r;
    t.col = col;
    t.isFreeze = false;  // 默认不是冰冻塔
    
    // 调整塔的属性
    if (ch == '1') {  // Arrow 塔
        t.damage = 1;       // 低伤害
        t.isAOE = false;    // 单体攻击
        t.rate = 3.0;       // 高攻速，每秒3次攻击
    } else if (ch == '2') {  // Arc 塔
        t.damage = 5;       // 将伤害从7降低到5
        t.isAOE = true;     // 保持AOE特性
        t.rate = 0.8;       // 低攻速，每秒0.8次攻击
    } else {  // Freeze 塔
        t.damage = 2;       // 中等伤害
        t.isAOE = false;    // 单体攻击
        t.isFreeze = true;  // 设置为冰冻塔
        t.rate = 1.0;       // 攻速，每秒1.0次攻击
        t.slowFactor = 0.5; // 减速50%
    }
    
    t.cooldown = 0.0;
    towers.push_back(t);
    playerGold -= cost;
}

// 添加难度选择菜单
void difficultyMenu() {
    clearScreen();
    cout << "=== 选择游戏难度 ===\n\n";
    cout << "1. 简单 - 怪物生命值降低，生成速度慢，初始资源丰富\n";
    cout << "2. 普通 - 平衡的游戏体验\n";
    cout << "3. 困难 - 怪物生命值提高，生成速度快，初始资源有限\n\n";
    cout << "请选择难度 (1-3): ";
    
    char choice;
    do {
        choice = cin.get();
        if (choice != '1' && choice != '2' && choice != '3') {
            cout << "无效输入，请重新选择 (1-3): ";
        }
    } while (choice != '1' && choice != '2' && choice != '3');
    
    flushStdin();
    
    switch (choice) {
        case '1': setDifficulty(EASY); break;
        case '2': setDifficulty(NORMAL); break;
        case '3': setDifficulty(HARD); break;
    }
}

// --- Main ---
int main() {
    srand(time(0));
    initGame();
    initPlusCols();
    
    cout << "1. 开始游戏  2. 退出: ";
    char mb = cin.get();
    if (mb != '1') return 0;
    flushStdin();
    
    // 添加难度选择
    difficultyMenu();
    
    const double fixedTimeStep = 1.0 / 30.0; // 30 FPS
    double accumulator = 0.0;
    auto currentTime = chrono::steady_clock::now();
    
    while (playerHP > 0 && waveNumber < maxWaves) {
        ++waveNumber;
        baseHP = 5 + (waveNumber - 1) * 2;
        toSpawn = waveNumber * 5;

        while (toSpawn > 0 || any_of(monsters.begin(), monsters.end(), [](auto& m) { return m.alive; })) {
            // 1) 如果有按键
            if (_kbhit()) {
                char cmd = _getch();
                if (cmd == 27) { // ESC key
                    clearScreen();
                    cout << "Game Exit\n";
                    exit(0);  // 直接退出程序
                }
                else if (cmd == 'b') {
                    placeTower();
                    flushStdin();
                }
                else if (cmd == 'p' || cmd == 'P') {
                    paused = !paused;
                }
            }

            // 2) 如果暂停，只绘制，不更新逻辑
            if (paused) {
                drawBuffer();
                this_thread::sleep_for(chrono::milliseconds(100));
                continue;
            }

            // 3) 计算时间增量
            auto newTime = chrono::steady_clock::now();
            double frameTime = chrono::duration<double>(newTime - currentTime).count();
            currentTime = newTime;
            
            // 限制最大帧时间，防止大延迟导致游戏跳跃
            if (frameTime > 0.25) frameTime = 0.25;
            
            accumulator += frameTime;
            
            // 使用固定时间步长更新游戏逻辑
            while (accumulator >= fixedTimeStep) {
                updateGame(fixedTimeStep);
                accumulator -= fixedTimeStep;
            }
            
            // 渲染
            drawBuffer();
            
            // 计算剩余时间并休眠
            auto endTime = chrono::steady_clock::now();
            auto sleepTime = chrono::duration_cast<chrono::milliseconds>(
                chrono::duration<double>(fixedTimeStep) - (endTime - newTime));
            if (sleepTime.count() > 0) {
                this_thread::sleep_for(sleepTime);
            }
        }
    }

    // 修改游戏结束显示，区分胜利和失败
    clearScreen();
    cout << "=== 游戏结束 ===\n\n";
    
    if (playerHP > 0) {
        cout << "恭喜你! 你成功击退了所有 " << maxWaves << " 波怪物的进攻!\n";
        cout << "你是塔防大师!\n\n";
    } else {
        cout << "你坚持了 " << waveNumber << " 波怪物的进攻，但最终被击败了。\n";
        cout << "再接再厉!\n\n";
    }
    
    cout << "难度: " << (gameDifficulty == EASY ? "简单" : (gameDifficulty == NORMAL ? "普通" : "困难")) << "\n\n";
    cout << "按任意键退出...";
    _getch();
    
    return 0;
}

void updateGame(double dt) {
    // 批量更新所有游戏对象
    spawnMon();
    moveMon();
    updateTowers(dt);
    updateProjectiles();
    
    // 使用更高效的方式删除死亡的怪物
    auto deadMonsterIt = remove_if(monsters.begin(), monsters.end(),
        [](const Monster& m) { return !m.alive; });
    if (deadMonsterIt != monsters.end()) {
        monsters.erase(deadMonsterIt, monsters.end());
    }
}
