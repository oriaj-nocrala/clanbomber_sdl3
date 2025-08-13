cmake_minimum_required(VERSION 3.16)
project(ClanBomberModern CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows compatibility fixes
if(WIN32)
    add_compile_definitions(_USE_MATH_DEFINES)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
    add_compile_definitions(M_PI=3.14159265358979323846)
    add_compile_definitions(M_PI_2=1.57079632679489661923)
    add_compile_definitions(M_PI_4=0.78539816339744830962)
    # Ensure we include math defines before any math headers
    add_compile_options(/FI"math.h")
endif()

# Include FetchContent for dependency management
include(FetchContent)

find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED) 
find_package(SDL3_ttf REQUIRED)
find_package(OpenGL REQUIRED)

# Enable C for GLAD
enable_language(C)

# Fetch cglm - modern dependency management
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG        v0.9.4
)

FetchContent_MakeAvailable(cglm)

# Fetch GLAD - OpenGL loader (using older version that works better with FetchContent)
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v0.1.36
)

FetchContent_MakeAvailable(glad)

# Configure GLAD
if(NOT TARGET glad)
    # Create GLAD target manually if not available
    add_library(glad STATIC ${glad_SOURCE_DIR}/src/glad.c)
    target_include_directories(glad PUBLIC ${glad_SOURCE_DIR}/include)
endif()

# STB is header-only, we'll include it directly
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)

FetchContent_MakeAvailable(stb)

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

target_link_libraries(clanbomber-modern PRIVATE 
    SDL3::SDL3 
    SDL3_image::SDL3_image 
    SDL3_ttf::SDL3_ttf 
    OpenGL::GL
    cglm
    glad
)

target_include_directories(clanbomber-modern PRIVATE 
    ${stb_SOURCE_DIR}
    ${glad_SOURCE_DIR}/include
)

# Copy data directory to build folder
file(COPY data DESTINATION ${PROJECT_BINARY_DIR})

# Windows-specific: Copy SDL3 DLLs to output directory
if(WIN32)
    # Copy data to Release/Debug directories as well
    file(COPY data DESTINATION ${PROJECT_BINARY_DIR}/Release)
    file(COPY data DESTINATION ${PROJECT_BINARY_DIR}/Debug)
    
    # Find and copy SDL3 DLLs
    find_file(SDL3_DLL NAMES SDL3.dll PATHS ${SDL3_DIR}/bin ${SDL3_DIR}/../bin PATH_SUFFIXES bin)
    find_file(SDL3_IMAGE_DLL NAMES SDL3_image.dll PATHS ${SDL3_image_DIR}/bin ${SDL3_image_DIR}/../bin PATH_SUFFIXES bin)
    find_file(SDL3_TTF_DLL NAMES SDL3_ttf.dll PATHS ${SDL3_ttf_DIR}/bin ${SDL3_ttf_DIR}/../bin PATH_SUFFIXES bin)
    
    if(SDL3_DLL)
        add_custom_command(TARGET clanbomber-modern POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL3_DLL} $<TARGET_FILE_DIR:clanbomber-modern>)
    endif()
    
    if(SDL3_IMAGE_DLL)
        add_custom_command(TARGET clanbomber-modern POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL3_IMAGE_DLL} $<TARGET_FILE_DIR:clanbomber-modern>)
    endif()
    
    if(SDL3_TTF_DLL)
        add_custom_command(TARGET clanbomber-modern POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${SDL3_TTF_DLL} $<TARGET_FILE_DIR:clanbomber-modern>)
    endif()
    
    # Alternative: copy data using post-build command to ensure it goes to the right place
    add_custom_command(TARGET clanbomber-modern POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/data $<TARGET_FILE_DIR:clanbomber-modern>/data)
endif()
