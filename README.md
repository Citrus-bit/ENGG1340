# ENGG1340
Tower Defense Game

Table of Contents

        Project Overview
        
        Gameplay
        
        Features
        
        Directory Structure
        
        Build & Run
        
        Dynamic Memory Management
        
        Save Module
        
        Contributors

Project Overview

        This is a console-based tower defense game implemented in C++. Players place different types of towers to intercept randomly spawning monsters across multiple waves. The game supports three difficulty levels and includes a binary save/load feature.
    
Gameplay

        Placement Phase: At the start of each wave, you have time to build towers. Press B (or select the menu) to place Arrow or Arc towers at marked '+' positions. Each tower costs gold.
        
        Wave Phase: Press Enter to begin. Monsters spawn from the leftmost 'S' markers and follow a segmented path towards the right. Surviving monsters decrease your HP when they reach the end.
        
        Combat: Towers automatically fire projectiles at nearby monsters. Arrow towers deal direct damage; Arc towers hit multiple targets. Frost traps and other items can slow or damage groups.
        
        Economy: Defeating monsters awards gold. Use gold in the shop between waves to buy upgrades or prepare special items.
        
        Shop & Inventory: After each wave, enter the shop to purchase immediate or prepare-type items. Prepare items take effect in the next placement phase.
        
        Save/Load: You can save progress to one of three slots and load it later from the main menu.
        
Features

        Random Events: Elite monster spawns, shop item order, and other events are randomized using PRNG.
        
        Multiple Difficulty Levels: Easy, Normal, and Hard modes affect monster count and attributes.
        
        Core Tower Defense Mechanics:
        
        Arrow Towers, Arc Towers, Frost Traps, and more.
        
        Monster pathing and segmented movement logic.
        
        Projectile movement and hit detection.
        
        Save Module:
        
        Binary save and load functions to persist game state.
        
        Dynamic Memory Management:
        
        Uses std::vector to manage dynamic lists of monsters, towers, and projectiles.

Directory Structure
        
        project_root/
        ├── main.cpp      # Game logic, UI rendering, and main loop
        ├── io.h          # Save/load interface declaration
        ├── io.cpp        # Save/load implementation (binary I/O)
        ├── Makefile      # Build and run script
        └── README.md     # Project documentation

Build & Run

        Linux & macOS
        
                Ensure you have a C++17 compiler installed (g++ or clang++) and make.
                
                Open a terminal, cd to the project root directory.
                
                Build with:
                
                        make
                
                Run the game:
                
                        ./tower_defense
                
                To clean build artifacts:
                
                        make clean
                
                Alternative (no Makefile)
                
                        g++ -std=c++17 main.cpp io.cpp -o tower_defense
                        ./tower_defense
        
        Windows (MinGW-w64)
        
                Install MinGW-w64 and add its bin folder to your PATH.
                
                Open Command Prompt or PowerShell and navigate to the project folder.
                
                Build with:
                
                        mingw32-make
                
                Or compile directly:
                
                        g++ -std=c++17 main.cpp io.cpp -o tower_defense.exe
                
                Run:
                
                        .	ower_defense.exe
                
                Clean up:
                
                        mingw32-make clean
        
        Windows (MSVC)
        
                Open “x64 Native Tools Command Prompt for VS”.
                
                Navigate to the project directory.
                
                Compile with:
                
                        cl /EHsc /std:c++17 main.cpp io.cpp /Fe:tower_defense.exe
                
                Run:
                        tower_defense.exe

Dynamic Memory Management

        All variable-length data structures (e.g., std::vector<Monster>, std::vector<Tower>, std::vector<Projectile>) rely on STL containers, which allocate and free memory on the heap automatically.

Save Module

        Files: io.h and io.cpp
        
        Interface:
        
        void saveGame(int slot, int difficulty, int playerHP, int playerGold, int waveNumber);
        
        bool loadGame(int slot, int& difficulty, int& playerHP, int& playerGold, int& waveNumber);
        
        Save paths: slot1.dat, slot2.dat, slot3.dat
        
        Uses binary I/O for data integrity.

Contributors

        LEUNG KA MING 3036453021
        LU SI NAN 3036452390 
        ZHU YONG KAI 3036457259 
        Tam Pou Seng 3036440854
