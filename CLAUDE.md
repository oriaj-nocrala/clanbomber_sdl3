# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

ClanBomber Modern is a modernized SDL3 port of the classic ClanBomber game. The codebase is being migrated from C++98 to modern C++17, featuring:

- **Core Engine**: Modern SDL3-based game engine with GPU acceleration
- **Rendering**: Advanced OpenGL 4.6 renderer with batched sprite rendering and particle systems
- **Game Logic**: Frame-rate independent deltaTime-based game loop
- **Architecture**: Modular design with clear separation between game logic, rendering, and input

## Build Commands

### Standard Build Process
```bash
cd build
cmake ..
make
./clanbomber-modern
```

### Development Build (Debug)
The project defaults to Debug mode with `-g -O0` flags for debugging.

### Dependencies
Required packages are fetched automatically via CMake FetchContent:
- SDL3, SDL3_image, SDL3_ttf (main SDL libraries)
- OpenGL (graphics)
- cglm (math library)
- GLAD v2 (OpenGL loader - auto-generated)
- STB (image loading utilities)

## Architecture Overview

### Core Classes

**ClanBomberApplication** (`src/ClanBomber.h`): Main application class managing game state, multiplayer networking, and core game loop. Handles both local and networked games.

**Game** (`src/Game.h`): Modern game wrapper providing screen management and SDL integration. Manages transitions between menu, gameplay, and settings screens.

**GPUAcceleratedRenderer** (`src/GPUAcceleratedRenderer.h`): Advanced OpenGL 4.6 renderer featuring:
- Batched sprite rendering with texture atlases
- GPU-accelerated particle system using compute shaders
- Advanced effects: explosions, heat distortion, environmental physics
- Optimized for thousands of sprites and particles

**Resources** (`src/Resources.h`): Central resource management system handling textures, fonts, and GL texture metadata. Coordinates between SDL surfaces and OpenGL textures.

### Game Architecture Patterns

**Screen System**: Uses a state machine pattern with `Screen` base class. Screens include:
- `MainMenuScreen`: Game menu and navigation
- `GameplayScreen`: Active game with bombers, bombs, explosions
- `SettingsScreen`: Configuration and options

**Controller System**: Polymorphic input handling via `Controller` base class:
- `Controller_Keyboard`: Player keyboard input
- `Controller_AI_Smart`/`Controller_AI_Modern`: AI implementations

**GameObject Hierarchy**: Game entities inherit from `GameObject`:
- `Bomber`: Player characters with movement, bombing, health
- `Bomb`/`ThrownBomb`: Explosive objects with timers
- `Explosion`: Damage areas with visual effects
- `Extra`: Power-ups (speed, bomb capacity, blast radius)

### Rendering Pipeline

1. **Resource Loading**: Textures loaded as SDL surfaces, converted to GL textures
2. **Batched Rendering**: Sprites accumulated in batches to minimize GL draw calls
3. **Effect Processing**: Special effects (explosions, particle systems) rendered with compute shaders
4. **Presentation**: Final frame presented via SDL3's GL context

### Key Technical Details

**OpenGL Context**: Managed through SDL3 with GLAD v2 for function loading. Context shared between main rendering and compute shaders.

**Texture Atlases**: Sprites stored in atlases with metadata for UV coordinate calculation. Supports animated sprites through numbered frames.

**Particle System**: GPU-based with SSBO (Shader Storage Buffer Objects) for particle data. Supports physics simulation including gravity, wind, collisions.

**Audio System**: Multi-channel audio mixer (`AudioMixer`) with SDL3 audio backend.

## Directory Structure

- `src/`: All C++ source files
- `src/shaders/`: OpenGL GLSL shader files  
- `data/`: Game assets (textures, sounds, maps)
- `build/`: CMake build directory (included in repo)

## Development Notes

### Shader Development
Shaders are copied to build directory automatically. Main shaders:
- `optimized_vertex_simple.glsl`: Vertex shader for sprite batching
- `optimized_fragment_simple.glsl`: Fragment shader with effect support
- `optimized_compute.glsl`: Particle system compute shader

### GPU Debugging
The renderer includes debug overlays and performance statistics. Enable with `GPUAcceleratedRenderer::enable_debug_overlay(true)`.

### Multiplayer Architecture
The game supports both local multiplayer and networked play through `Server`/`Client` classes. Network code handles bomber synchronization and bomb activation across clients.
- Para compilar siempre nos cercioramos de que estemos en el directorio build. Para ello podemos usar pwd. Luego, en el directorio build, hacemos cmake .. && make -j24