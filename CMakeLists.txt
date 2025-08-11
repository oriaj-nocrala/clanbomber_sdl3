cmake_minimum_required(VERSION 3.16)
project(ClanBomberModern CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED)
find_package(SDL3_ttf REQUIRED)

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
    src/MapTile_Wall.cpp
    src/MapTile_Ground.cpp
    src/Audio.cpp
    src/Game.cpp
    src/MainMenuScreen.cpp
    src/GameplayScreen.cpp
    src/SettingsScreen.cpp
)

target_link_libraries(clanbomber-modern PRIVATE SDL3::SDL3 SDL3_image::SDL3_image SDL3_ttf::SDL3_ttf)

file(COPY data DESTINATION ${PROJECT_BINARY_DIR})
