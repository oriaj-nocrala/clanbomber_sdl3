cmake_minimum_required(VERSION 3.16)
project(ClanBomberModern CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED) 
find_package(SDL3_ttf REQUIRED)
find_package(OpenGL REQUIRED)

# Enable C for GLAD
enable_language(C)

# Add local libraries - cglm as subdirectory
add_subdirectory(lib/cglm)

# GLAD sources
set(GLAD_SOURCES lib/glad/src/glad.c)
set(GLAD_INCLUDES lib/glad/include)

# STB header-only libraries  
set(STB_INCLUDES lib/stb)

add_executable(clanbomber-modern 
    src/main.cpp
    src/ClanBomber.cpp
    src/GameConfig.cpp
    src/Controller.cpp
    src/GameObject.cpp
    src/Resources.cpp
    src/Bomber.cpp
    src/Controller_Keyboard.cpp
    src/Bomb.cpp
    src/Explosion.cpp
    src/Timer.cpp
    src/Map.cpp
    src/MapEntry.cpp
    src/MapTile.cpp
    src/MapTile_Wall.cpp
    src/MapTile_Ground.cpp
    src/MapTile_Box.cpp
    src/AudioMixer.cpp
    src/Extra.cpp
    src/CorpsePart.cpp
    src/BomberCorpse.cpp
    src/ParticleSystem.cpp
    src/GPURenderer.cpp
    src/GPUAcceleratedRenderer.cpp
    src/GPUAcceleratedRenderer_impl.cpp
    src/Game.cpp
    src/MainMenuScreen.cpp
    src/GameplayScreen.cpp
    src/SettingsScreen.cpp
)

target_sources(clanbomber-modern PRIVATE ${GLAD_SOURCES})

target_link_libraries(clanbomber-modern PRIVATE 
    SDL3::SDL3 
    SDL3_image::SDL3_image 
    SDL3_ttf::SDL3_ttf 
    OpenGL::GL
    cglm
)

target_include_directories(clanbomber-modern PRIVATE 
    ${GLAD_INCLUDES}
    ${STB_INCLUDES}
    lib/cglm/include
)

file(COPY data DESTINATION ${PROJECT_BINARY_DIR})
