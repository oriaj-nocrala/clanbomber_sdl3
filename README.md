# ClanBomber Modern (SDL3 Port)

This is a work-in-progress modernization of the classic game ClanBomber. The goal is to port the original C++98 codebase to modern C++, using SDL3 and removing old dependencies like Boost.

## Dependencies

- SDL3
- SDL3_image
- SDL3_ttf
- A C++ compiler that supports C++17 or newer
- CMake

## How to Build

```bash
# Clone the repository
git clone https://github.com/oriaj-nocrala/clanbomber_sdl3.git
cd clanbomber_sdl3

# Configure and build the project
# (The build directory is included in the repo for convenience, but creating a new one is also fine)
cd build
cmake ..
make

# Run the game
./clanbomber-modern
```

## Status

This project is currently under active development. The core game logic is being refactored to be frame-rate independent using a deltaTime-based game loop.
