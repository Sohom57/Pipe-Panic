<div align="center">

# 🎮 PIPE PANIC
### A glowing ball. A twisting 3D maze. One way out.

**Find the exit, dodge the traps, and see how far you can go.**

![Platform](https://img.shields.io/badge/platform-Windows-2563eb?style=for-the-badge&logo=windows&logoColor=white)
![Language](https://img.shields.io/badge/language-C%2B%2B-f97316?style=for-the-badge&logo=cplusplus&logoColor=white)
![Graphics](https://img.shields.io/badge/graphics-OpenGL-8b5cf6?style=for-the-badge)

</div>

---

## ✨ The game

Pipe Panic is a 3D maze game built with C++ and OpenGL. Guide a golden ball
through a transparent pipe maze, avoid glowing traps, and reach the green exit.
Every level generates a new maze, and the challenge ramps up as you progress.

## ▶️ Gameplay demo

<div align="center">

<video src="./Screen%20Recording%202026-10-21%20110546.mp4" controls width="800">
  Your browser does not support embedded video.
</video>

[▶ Open or download the gameplay demo](./Screen%20Recording%202026-10-21%20110546.mp4)

</div>

## 🚀 Play on Windows

1. Download or clone [`Sohom57/Pipe-Panic`](https://github.com/Sohom57/Pipe-Panic).
2. Open `bin/Debug/Khela.exe` to start the game.
3. Keep `bin/Debug/glut32.dll` beside the executable. It is included in the
   repository.

> **Tip:** If you move the executable, move `glut32.dll` with it. The game
> requires the GLUT runtime and Windows OpenGL.

## 🕹️ Controls

<details>
<summary><strong>Movement · Rotate the ball through the maze</strong></summary>

| Keys | Movement |
|:---:|---|
| <kbd>W</kbd> / <kbd>S</kbd> | −Z / +Z |
| <kbd>A</kbd> / <kbd>D</kbd> | −X / +X |
| <kbd>Q</kbd> / <kbd>E</kbd> | −Y / +Y |

</details>

<details>
<summary><strong>View · Inspect the maze from every angle</strong></summary>

| Keys | Action |
|:---:|---|
| <kbd>I</kbd> / <kbd>K</kbd> | Rotate maze around X |
| <kbd>J</kbd> / <kbd>L</kbd> | Rotate maze around Y |
| <kbd>U</kbd> / <kbd>O</kbd> | Rotate maze around Z |
| <kbd>←</kbd> / <kbd>→</kbd> | Orbit camera horizontally |
| <kbd>↑</kbd> / <kbd>↓</kbd> | Orbit camera vertically |
| <kbd>+</kbd> / <kbd>−</kbd> or <kbd>Page Up</kbd> / <kbd>Page Down</kbd> | Zoom |
| <kbd>T</kbd> | Reset the view |

</details>

<details>
<summary><strong>Game · Choose a challenge or start again</strong></summary>

| Key | Action |
|:---:|---|
| <kbd>1</kbd> | Start an easy game |
| <kbd>2</kbd> | Start a medium game |
| <kbd>3</kbd> | Start a hard game |
| <kbd>N</kbd> | Generate the next level |
| <kbd>R</kbd> | Retry the current level |
| <kbd>Esc</kbd> | Quit |

</details>

## 🧩 What to expect

- **Fresh mazes:** procedurally carved 3D routes, with loops to keep the path
  interesting.
- **A growing challenge:** choose a starting difficulty; later levels add
  complexity and traps.
- **A little atmosphere:** transparent pipe tunnels, glowing hazards, a pulsing
  exit, and a golden ball trail.
- **Full 3D control:** rotate the maze and orbit or zoom the camera while you
  plan your route.

## 🛠️ Build from source

The project includes the Code::Blocks project file [`Khela.cbp`](./Khela.cbp)
and source code in [`main.cpp`](./main.cpp). Open the project in Code::Blocks
with a MinGW GCC toolchain and the GLUT, OpenGL, GLU, and WinMM libraries
available, then build the **Release** or **Debug** target.

---

<div align="center">

**Can you find the exit?**  
Made with C++, OpenGL, and a little patience. ✨

</div>
