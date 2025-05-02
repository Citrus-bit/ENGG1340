#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <thread>
#include <cctype>
#include <termios.h> // 替换 <conio.h>
#include <unistd.h>
#include <fcntl.h>
using namespace std;

// ─── Map ─────────────────────────────────────────
vector<string> baseMap = {
"   A   B   C   D   E   F   G   H   I   J   K   L   M   ",
"1  +   +   +   +   +   +   +   +   +   +   +   +   +   ",
"--------------------------------------------------------",
"S                                                      |",
"S                                                      |",
"S                                                      |",
"S                                                      |",
"S                                                      |",
"--------------------------------------------------------",
"2  +   +   +   +   +   +   +   +   +   +   +   +   +   ",
"--------------------------------------------------------",
"|                                                      |",
"|                                                      |",
"|                                                      |",
"|                                                      |",
"|                                                      |",
"--------------------------------------------------------",
"3  +   +   +   +   +   +   +   +   +   +   +   +   +   ",
"--------------------------------------------------------",
"|                                                      H",
"|                                                      H",
"|                                                      H",
"|                                                      H",
"|                                                      H",
"--------------------------------------------------------",
"4  +   +   +   +   +   +   +   +   +   +   +   +   +   "
};

// ─── Structures ─────────────────────────────────────
struct Monster {
    int r, c; int hp; bool elite, alive;
};
struct Tower {
    int r, c; int dmg; bool aoe;
    double rate, cd;   // 次/秒 & 倒计时
    bool hasBullet;   // 关键标志

    // Constructor for Tower
    Tower(int r, int c, int dmg, bool aoe, double rate, double cd, bool hasBullet)
        : r(r), c(c), dmg(dmg), aoe(aoe), rate(rate * 5), cd(cd), hasBullet(hasBullet) {}
};
struct Bullet {
    vector<pair<int, int> > path; size_t idx;
    int owner; char ch; int dmg;
};

// ─── Globals ──────────────────────────────────────
vector<int> plusRows = { 1, 9, 17, 25 };
vector<int> plusCols;
vector<Monster> mobs;
vector<Tower>   towers;
vector<Bullet>  bullets;
int hp = 10, gold = 100, wave = 0, tospawn = 0, spawnCD = 0;
void clear() { system("clear"); } // 替换 ANSI 转义序列

// ─── Helper ───────────────────────────────────────
vector<pair<int, int> > bres(int r0, int c0, int r1, int c1) {
    vector<pair<int, int> > v; int dr = abs(r1 - r0), dc = abs(c1 - c0);
    int sr = r0 < r1 ? 1 : -1, sc = c0 < c1 ? 1 : -1, err = dr - dc;
    for (int r = r0, c = c0;;) {
        v.push_back({ r,c }); if (r == r1 && c == c1)break;
        int e2 = 2 * err;
        if (e2 > -dc) { err -= dc; r += sr; }
        if (e2 < dr) { err += dr; c += sc; }
    }
    return v;
}
void initCols() {
    plusCols.clear(); // 确保清空之前的内容
    for (int i = 0; i < baseMap[1].size(); ++i) { // 修正为从第2行提取列索引
        if (baseMap[1][i] == '+') {
            plusCols.push_back(i);
        }
    }
}

// ─── Input Handling ───────────────────────────────
// 替换 _kbhit 和 _getch
int kbhit() {
    struct termios oldt, newt;
    int ch;
    int oldf;

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    if (ch != EOF) {
        ungetc(ch, stdin);
        return 1;
    }

    return 0;
}

char getch() {
    struct termios oldt, newt;
    char ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// ─── Game Logic ───────────────────────────────────
void spawn() {
    if (tospawn && spawnCD == 0) {
        mobs.push_back(Monster{ 3 + rand() % 3, 0, 60, true, true }); // 提升普通怪血量为箭矢伤害的10倍（假设箭矢伤害为6）
        --tospawn; spawnCD = 6;
    }
    else if (spawnCD) --spawnCD;
}
void moveMobs() {
    for (auto& m : mobs) if (m.alive) {
        if (m.c == baseMap[m.r].size() - 1) {
            if (m.r >= 3 && m.r <= 7) {
                m.r += 8; // 移动到第11行
                m.c = 0;  // 重置列为起点
            } else if (m.r >= 11 && m.r <= 15) {
                m.r += 8; // 移动到第19行
                m.c = 0;  // 重置列为起点
            } else {
                m.alive = false;
                --hp;
                if (hp <= 0) {
                    cout << "Game Over\n";
                    exit(0); // 血量为0时立即结束游戏
                }
            }
        } else {
            if (rand() % 2 == 0) { // 降低移动速度
                ++m.c;
            }
        }
    }
}
Monster* nearestWithPrediction(const Tower& t) {
    Monster* best = nullptr;
    int bestd = 1e9;
    for (auto& m : mobs) if (m.alive) {
        int predicted_c = m.c + 2; // 简单预测敌人下一步移动位置
        if (predicted_c >= baseMap[m.r].size()) {
            predicted_c = baseMap[m.r].size() - 1; // 防止越界
        }
        int d = abs(m.r - t.r) + abs(predicted_c - t.c);
        if (d < bestd && d <= 10) { // 限制攻击范围为 10
            bestd = d;
            best = &m;
        }
    }return best;
}
void fire(int id, Tower& t) {
    if (t.hasBullet || t.cd > 0) return;
    auto* m = nearestWithPrediction(t);
    if (!m) {
        cout << "Tower " << id << " has no target.\n";
        return;
    }
    auto path = bres(t.r, t.c, m->r, m->c);
    bullets.push_back({ path, 0, id, t.aoe ? 'o' : '.', t.dmg });
    t.hasBullet = true;
    t.cd = 1.0 / t.rate;
    cout << "Tower " << id << " fired at target (" << m->r << ", " << m->c << ").\n";
}

void updateTowers(double dt) {
    for (size_t i = 0;i < towers.size();++i) {
        auto& t = towers[i]; if (t.cd > 0) t.cd -= dt;
        fire(i, t);
    }
}
void updateBullets() {
    for (auto& b : bullets) {
        if (b.idx < b.path.size()) {
            b.idx += (b.ch == 'o' ? 1 : 2); // AOE 炮弹移动速度慢，箭矢移动速度快
        }
    }
    for (auto& b : bullets) {
        if (b.idx >= b.path.size()) {
            towers[b.owner].hasBullet = false;
            continue;
        }
        auto [r, c] = b.path[b.idx];
        for (auto& m : mobs) if (m.alive) {
            if (towers[b.owner].aoe) {
                if (abs(m.r - r) <= 5 && abs(m.c - c) <= 5) { // AOE 范围 5x5
                    m.hp -= b.dmg;
                }
            } else {
                if (m.r == r && m.c == c) { // 单体攻击
                    m.hp -= b.dmg;
                }
            }
            if (m.hp <= 0) {
                m.alive = false;
                gold += m.elite ? 5 : 2; // 减少金币奖励
            }
            b.idx = b.path.size();
            towers[b.owner].hasBullet = false;
            break;
        }
    }
    bullets.erase(remove_if(bullets.begin(), bullets.end(),
        [](auto& b) {return b.idx >= b.path.size();}), bullets.end());
}

// ─── Render ─────────────────────────────────────────
void draw() {
    clear();
    auto buf = baseMap;
    for (auto& t : towers) buf[t.r][t.c] = t.aoe ? 'C' : 'A';
    for (auto& m : mobs) if (m.alive) buf[m.r][m.c] = 'M';
    for (auto& b : bullets) if (b.idx < b.path.size()) {
        auto [r, c] = b.path[b.idx];
        buf[r][c] = b.ch; // 显示子弹的运动轨迹
    }
    for (int r = 0; r < buf.size(); ++r) {
        bool show = find(plusRows.begin(), plusRows.end(), r) != plusRows.end();
        if (show) cout << setw(3) << r << ' '; else cout << "    ";
        cout << buf[r] << "\n";
    }
    cout << "HP:" << hp << "  Gold:" << gold << "  Wave:" << wave << "\n";
}

// ─── Tower Place ───────────────────────────────────
void place() {
    char ch; do { cout << "\n1)A(20) 2)C(30):";cin >> ch; } while (ch != '1' && ch != '2');
    int cost = ch == '2' ? 30 : 20; if (gold < cost) { cout << "no $";return; }
    // 修改行选择逻辑
    int r; 
    do { 
        cout << "Row(1/2/3/4):"; 
        cin >> r; 
        if (cin.fail()) { // 检查输入流是否失败
            cin.clear(); // 清除错误状态
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 丢弃无效输入
            r = -1; // 设置为无效值
            cout << "Invalid input. Please enter a number (1, 2, 3, or 4).\n";
        } else {
            switch (r) {
                case 1: r = 1; break;
                case 2: r = 9; break;
                case 3: r = 17; break;
                case 4: r = 25; break;
                default: r = -1; cout << "Invalid row. Please choose 1, 2, 3, or 4.\n"; break;
            }
        }
    } while (r == -1);

    // 修改列输入校验逻辑
    char lc; 
    int idx; 
    do {
        cout << "Col label (字母提示：A-M):";
        cin >> lc;
        if (cin.fail() || !isalpha(lc)) { // 检查输入流是否失败或非字母
            cin.clear(); // 清除错误状态
            cin.ignore(numeric_limits<streamsize>::max(), '\n'); // 丢弃无效输入
            idx = -1; // 设置为无效值
            cout << "Invalid input. Please enter a valid letter (A-M).\n";
        } else {
            lc = toupper(lc); // 转换为大写
            idx = lc - 'A';
            if (idx < 0 || idx >= plusCols.size()) {
                cout << "Invalid column. Please choose a valid letter (A-M).\n";
            }
        }
    } while (idx < 0 || idx >= plusCols.size());

    // 根据塔类型设置特性
    if (ch == '2') { // C塔
        towers.push_back({ r, plusCols[idx], 5, true, 0.5, 0, false }); // 攻速慢，伤害高，范围大
    } else { // A塔
        towers.push_back({ r, plusCols[idx], 2, false, 2.0, 0, false }); // 攻速快，伤害低，范围小
    }
    gold -= cost;
}

void waitForKeyPress() {
    cout << "按下 'q' 键以继续游戏..." << endl;
    while (true) {
        if (kbhit()) {
            char key = getch();
            if (key == 'q') {
                break;
            }
        }
    }
}

// ─── Main ───────────────────────────────────────────
int main() {
    srand(time(0));
    initCols();

main_menu:
    cout << "1) 新游戏\n2) 加载游戏\n3) 新手教学\n";
    char choice;
    cin >> choice; // 修复未初始化的变量
    if (choice == '3') {
        cout << "新手教学功能尚未实现。\n";
        return 0;
    } else if (choice != '1') {
        return 0;
    }

    auto last = chrono::steady_clock::now();
    while (hp > 0) {
        ++wave; tospawn = wave * 5;
        while (tospawn || any_of(mobs.begin(), mobs.end(), [](auto& m) {return m.alive;})) {
            auto now = chrono::steady_clock::now();
            double dt = chrono::duration<double>(now - last).count(); last = now;
            spawn(); moveMobs(); updateTowers(dt); updateBullets(); draw();
            if (kbhit()) {
                char key1 = getch();
                if (kbhit()) { // 检测是否有第二个键按下
                    char key2 = getch();
                    if ((key1 == 'x' && key2 == 'y') || (key1 == 'y' && key2 == 'x')) { // 检测按键组合
                        cout << "检测到强制返回主界面组合键，返回主界面...\n";
                        mobs.clear(); // 清空怪物
                        bullets.clear(); // 清空子弹
                        towers.clear(); // 清空塔
                        hp = 10; gold = 100; wave = 0; tospawn = 0; spawnCD = 0; // 重置游戏状态
                        goto main_menu; // 跳转到主界面
                    }
                }
                if (key1 == 'b') place(); // 替换 _kbhit 和 _getch
            }
            this_thread::sleep_for(chrono::milliseconds(100));
            mobs.erase(remove_if(mobs.begin(), mobs.end(), [](auto& m) {return !m.alive;}), mobs.end());
        }
        waitForKeyPress(); // 等待用户按下 'q' 键
    }
    cout << "Game Over\n";
    return 0;
}
