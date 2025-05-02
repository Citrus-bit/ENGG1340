# ENGG1340
Tower Defense Game

Table of Contents

        Project Overview
        
        Features
        
        Directory Structure
        
        Build & Run
        
        Dynamic Memory Management
        
        Save Module
        
        Contributors

Project Overview

    This is a console-based tower defense game implemented in C++. Players place different types of towers to intercept randomly spawning monsters across multiple waves. The game supports three difficulty levels and includes a binary save/load feature.

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

        Ensure you have g++ (C++17) installed.
        
        In the project root directory, run:
        
        make
        
        After compilation, launch the game with:
        
        ./tower_defense
        
        You can also run directly:
        
        make run
        
        To remove build artifacts:
        
        make clean

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
