# ClanBomber SDL3 - Build Instructions

## Prerequisites

### All Platforms
- CMake 3.16 or higher
- C++17 compatible compiler
- Git (for dependency fetching)

### Windows
- Visual Studio 2019/2022 OR MinGW-w64
- SDL3 development libraries
- SDL3_image development libraries  
- SDL3_ttf development libraries

### Linux
- GCC/Clang
- SDL3 development packages
- OpenGL development headers

## Dependencies

This project now uses **CMake FetchContent** for dependency management. The following libraries are automatically downloaded and built:

- **cglm v0.9.4** - OpenGL Mathematics library
- **GLAD v2.0.8** - OpenGL loader
- **stb** - Image loading (header-only)

## Build Steps

### 1. Clone Repository
```bash
git clone https://github.com/oriaj-nocrala/clanbomber_sdl3.git
cd clanbomber_sdl3
```

### 2. Configure Build
```bash
mkdir build
cd build
cmake ..
```

### 3. Build
```bash
# All platforms
cmake --build .

# Or with specific configuration on Windows
cmake --build . --config Release
```

## Windows-Specific Notes

### DLL Handling
The build system automatically:
- Copies SDL3 DLLs to the executable directory
- Copies the `data/` folder to Release/Debug folders
- Handles Windows-specific math constant definitions

### Visual Studio
When using Visual Studio, build artifacts will be in:
- `build/Release/` for Release builds
- `build/Debug/` for Debug builds

The game data and DLLs are automatically copied to these directories.

### Alternative SDL3 Installation
If CMake cannot find SDL3, you can:
1. Download SDL3 development libraries from [libsdl.org](https://www.libsdl.org)
2. Set CMAKE_PREFIX_PATH to point to your SDL3 installation:
```bash
cmake -DCMAKE_PREFIX_PATH="C:/path/to/SDL3" ..
```

## Linux Package Requirements

### Ubuntu/Debian
```bash
sudo apt install cmake build-essential libsdl3-dev libsdl3-image-dev libsdl3-ttf-dev libgl1-mesa-dev
```

### Fedora
```bash
sudo dnf install cmake gcc-c++ SDL3-devel SDL3_image-devel SDL3_ttf-devel mesa-libGL-devel
```

### Arch Linux
```bash
sudo pacman -S cmake gcc sdl3 sdl3_image sdl3_ttf mesa
```

## GPU Acceleration

The game includes OpenGL 4.6 GPU acceleration with:
- Compute shaders for 50,000+ particle physics
- Optimized vertex/fragment shaders
- Modern rendering pipeline

Ensure your graphics drivers support OpenGL 4.6 for optimal performance.

## Troubleshooting

### "M_PI not defined" on Windows
This is automatically handled by the build system. If you still see this error, ensure you're using the latest CMakeLists.txt.

### Missing DLLs on Windows
The build system should copy DLLs automatically. If missing:
1. Check that SDL3 is properly installed
2. Manually copy DLLs from SDL3 installation to executable directory

### Data folder not found
The `data/` folder should be automatically copied. If not:
1. Manually copy `data/` folder to your build directory
2. Ensure you're running the executable from the correct directory

## Performance Optimization

For maximum performance, build in Release mode:
```bash
cmake --build . --config Release
```

This enables:
- Compiler optimizations
- GPU acceleration features
- Optimized particle systems