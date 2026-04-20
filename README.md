# 🌍 Eidos Engine (ProjectGAF)

![C++](https://img.shields.io/badge/Language-C++20-blue.svg)
![Build](https://img.shields.io/badge/Build-CMake-success.svg)
![Status](https://img.shields.io/badge/Status-Alpha_0.0.1-orange.svg)
![Developer](https://img.shields.io/badge/Developer-Solo-purple.svg)

Welcome to **Eidos Engine** — the technical foundation for **ProjectGAF**, an infinitely procedural **Voxel Evolution Sandbox** built entirely from scratch. 

This is a pure passion project developed by a **solo developer**. Instead of using ready-made commercial engines, I chose the hardcore path: writing a custom C++ engine to have absolute control over every byte of memory, every thread, and every rendered frame.

##  The Vision: From Stone Age to Space Age
**ProjectGAF** is not just another block-building game; it is a game about the ultimate technological progression. 
* **Stone Age Survival:** Start with primitive tools. Chip stones, gather branches, and survive the unforgiving, harsh environments (heavily inspired by *Vintage Story*).
* **Industrial Revolution:** Progress through the Bronze and Iron ages. Discover mechanical power, build complex multi-block machines, automate resource processing, and wire up electricity.
* **Space Exploration:** Break the boundaries of your starting planet. Engineer rockets, manage fuel and life support, and explore the infinite procedural cosmos.

To support this massive scale of evolution—from rendering a campfire to simulating a factory—the engine is built with extreme performance in mind.

##  Core Engine Features
Writing a voxel engine in C++ is a battle against the CPU bottleneck. Eidos Engine tackles this with aggressive optimization:

* **Extreme Multithreading:** Terrain generation and mesh building are decoupled into strictly isolated thread queues.
* **Zero-Copy Mesh Swapping:** Chunk meshes are built in temporary background buffers and swapped into the main thread in a fraction of a millisecond (`std::swap`). Zero input lag when breaking or placing blocks.
* **Advanced Voxel Lighting:** Features a double-buffered smooth lighting system with Ambient Occlusion (AO). Light accurately flood-fills rooms, cascades through transparent blocks, and dynamically updates without "Dirty Read" artifacts or "Light Echoes".
* **Y-Culling (Sky Optimization):** The engine dynamically calculates the highest terrain point (`maxY`) per chunk, ignoring empty sky space to boost mesh generation speed by over 60%.
* **Smart Delta-Saving:** The engine tracks player interactions and saves *only* modified chunks to the disk as binary `.bin` files, paired with an automatic 60-second background save system.

##  Building from Source

### Prerequisites
* **C++20** compatible compiler (MSVC, GCC, or Clang)
* **CMake** (3.15 or higher)

### Build Instructions
```bash
# Clone the repository
git clone [https://github.com/YOUR_USERNAME/EidosEngine.git](https://github.com/YOUR_USERNAME/EidosEngine.git)
cd EidosEngine

# Generate build files
mkdir build
cd build
cmake ..

# Compile the project (Release mode is highly recommended for voxel engines)
cmake --build . --config Release

 Screenshots

(Add your awesome screenshots here! Create an assets/images folder in your repo, upload your screenshots, and link them like this:)
<img src="https://www.google.com/search?q=https://via.placeholder.com/400x225.png%3Ftext%3DGameplay%2BScreenshot%2B1" width="400">	<img src="https://www.google.com/search?q=https://via.placeholder.com/400x225.png%3Ftext%3DGameplay%2BScreenshot%2B2" width="400">
Infinite procedural terrain with smooth lighting	In-game Lua console and Waila block overlay
 About the Developer

My name is Bohdan, and I am the sole developer behind this project. I am building Eidos Engine to push my C++ skills to the absolute limit and to prove that one person with enough dedication can build the core tech for a massive evolution sandbox from the ground up.
 License

This project is for personal portfolio and educational purposes. All rights reserved.
