cmake_minimum_required(VERSION 3.16)
project(ClanBomberModern CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Debug flags for better gdb experience
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -fno-omit-frame-pointer")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "")

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

# --- DEPENDENCIAS PRINCIPALES ---
find_package(SDL3 REQUIRED)
find_package(SDL3_image REQUIRED) 
find_package(SDL3_ttf REQUIRED)
find_package(OpenGL REQUIRED)
# AÑADIDO: Python es ahora un requerimiento para GLAD v2
find_package(Python REQUIRED)

# Habilitamos el lenguaje C, necesario para compilar las dependencias
enable_language(C)

# --- cglm (sin cambios) ---
FetchContent_Declare(
    cglm
    GIT_REPOSITORY https://github.com/recp/cglm.git
    GIT_TAG        v0.9.4
)
FetchContent_MakeAvailable(cglm)

# ===================================================================
#    BLOQUE ACTUALIZADO: CONTROL MANUAL COMPLETO DE GLAD v2.0.8
# Este bloque reemplaza la configuración antigua de GLAD v0.1.36
# ===================================================================
FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG        v2.0.8 # <-- ACTUALIZADO a la versión moderna
)
FetchContent_GetProperties(glad)
if(NOT glad_POPULATED)
    FetchContent_Populate(glad)
endif()

# Definimos las rutas para los archivos que vamos a generar
set(GLAD_GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/glad-generated)
set(GLAD_C_FILE ${GLAD_GENERATED_DIR}/src/gl.c) # <-- Nombre de archivo correcto: gl.c
set(GLAD_H_FILE_DIR ${GLAD_GENERATED_DIR}/include)

# Comando personalizado para ejecutar el generador de Python con la sintaxis correcta
add_custom_command(
    OUTPUT  ${GLAD_C_FILE}
    COMMAND ${Python_EXECUTABLE} -m glad
            --api="gl:core=4.6"
            --out-path=${GLAD_GENERATED_DIR}
            c
    WORKING_DIRECTORY ${glad_SOURCE_DIR}
    DEPENDS ${glad_SOURCE_DIR}/glad/files/gl.xml
    COMMENT "Generando el cargador de GLAD v2 con Python..."
)

# Creamos un target para el generador
add_custom_target(
    glad-generator
    DEPENDS ${GLAD_C_FILE}
)

# Creamos la biblioteca 'glad' a partir del archivo C generado
add_library(glad STATIC ${GLAD_C_FILE})

# Creamos la cadena de dependencias: glad depende del generador
add_dependencies(glad glad-generator)

# Le decimos a 'glad' dónde están sus propios headers y lo hacemos PÚBLICO
# para que nuestro ejecutable también lo sepa.
target_include_directories(glad
    PUBLIC
    ${GLAD_H_FILE_DIR}
)
# ===================================================================
#                   FIN DEL BLOQUE DE CONTROL MANUAL
# ===================================================================

# --- STB (sin cambios) ---
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
)
FetchContent_MakeAvailable(stb)

# --- EJECUTABLE ---
add_executable(clanbomber-modern 
    src/main.cpp
    src/ClanBomber.cpp
    src/GameConfig.cpp
    src/Controller.cpp
    src/GameObject.cpp
    src/Resources.cpp
    src/Bomber.cpp
    src/Controller_Keyboard.cpp
    src/Controller_AI_Smart.cpp
    src/Controller_AI_Modern.cpp
    src/Bomb.cpp
    src/ThrownBomb.cpp
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
    src/GPUAcceleratedRenderer.cpp
    src/TextRenderer.cpp
    src/Game.cpp
    src/MainMenuScreen.cpp
    src/GameplayScreen.cpp
    src/SettingsScreen.cpp
    src/LifecycleManager.cpp
    src/TileManager.cpp
)

# --- ENLACE DE BIBLIOTECAS ---
target_link_libraries(clanbomber-modern PRIVATE 
    SDL3::SDL3 
    SDL3_image::SDL3_image 
    SDL3_ttf::SDL3_ttf 
    OpenGL::GL
    cglm
    glad
)

# --- DIRECTORIOS DE INCLUSIÓN ---
# MODIFICADO: La inclusión de GLAD ahora es automática gracias a 'PUBLIC'.
target_include_directories(clanbomber-modern PRIVATE 
    ${stb_SOURCE_DIR}
)

# --- COPIA DE ARCHIVOS (sin cambios) ---
file(COPY data DESTINATION ${PROJECT_BINARY_DIR})

file(GLOB SHADER_FILES "src/shaders/*.glsl")
file(GLOB SHADER_BACKUP_FILES "src/shaders/*.bak")
file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/shaders)
file(COPY ${SHADER_FILES} ${SHADER_BACKUP_FILES} DESTINATION ${PROJECT_BINARY_DIR}/shaders)

# --- CONFIGURACIÓN PARA WINDOWS (sin cambios) ---
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
