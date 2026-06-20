# WAR-GAME
A 2D aerial combat game built with C++ and SFML featuring WW2 and Gulf War aircraft, missile combat, tank enemies, explosions, sound effects, and score-based gameplay.
# 🚀 Lockheed – 2D Aerial Combat Game (SFML)

A high-performance 2D aerial combat game built using **C++ and SFML**, featuring WWII and Gulf War aircraft combat, dynamic enemies, explosions, and real-time action gameplay.

---

## 🎮 Game Overview

Lockheed is a combat flight game where the player controls a fighter jet and engages in intense aerial warfare against tanks, missiles, and environmental hazards.

### 🧩 Game Modes
- 🇩🇪 WWII Mode
- 🇺🇸 Gulf War Mode

---

## ✨ Features

- Multiple playable jets with unique stats
- Enemy tank AI with shooting system
- Missile tracking system
- Bomb physics system
- Health + scoring system
- Explosion & smoke particle effects
- Background music & sound system
- Video cutscene system
- Dynamic difficulty scaling
- Pause / resume gameplay

---

## 🧰 Tech Stack

- C++
- SFML (Graphics, Window, System, Audio)
- MinGW / GCC (Dev-C++ compatible)

---

## 📁 Project Structure
Lockheed/
│
├── main.cpp
├── Assets/
│ ├── Backgrounds/
│ ├── Effects/
│ ├── Enemies/
│ ├── Jets/
│ ├── Sounds/
│ ├── UI/
│ ├── Videos/
│ └── Weapons/
│
├── README.md
└── .gitignore

---

## ⚙️ Setup Instructions

### 1. Install SFML (2.6.x recommended)
Download SFML:
https://www.sfml-dev.org/download.php

Make sure it matches your compiler (MinGW version).

---

### 2. Configure Dev-C++

Add these paths:

**Include:**
**Library:**

C:\SFML\lib


---

### 3. Linker Settings

Add:


-lsfml-graphics
-lsfml-window
-lsfml-system
-lsfml-audio


---

### 4. Copy DLL Files

From:

C:\SFML\bin


Copy all DLLs into your project executable folder.

---

## 🎮 Controls

| Key | Action |
|-----|--------|
| Arrow Keys | Move jet |
| Space | Drop bomb |
| Esc | Pause / Resume |
| Enter | Skip cutscene |
| H | WW2 Mode |
| B | Gulf Mode |
| A / B | Select aircraft |

---

## 📦 Requirements

- Windows OS
- SFML 2.x
- GCC / MinGW compatible compiler
- Assets folder must exist

---

## ⚠️ Important Notes

- The `Assets/` folder must be in the same directory as the `.exe`
- Missing assets will cause runtime errors
- Video system requires full `Videos/` directory structure
- Game will not run without SFML properly linked

---

## 📜 License

This project is for **educational and learning purposes only**.
