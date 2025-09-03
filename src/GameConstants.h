#pragma once

/**
 * @file GameConstants.h
 * @brief Centralized constants for ClanBomber to replace magic numbers
 * 
 * This file contains all the commonly used constants that were previously
 * hardcoded as magic numbers throughout the codebase. This improves
 * maintainability and makes the code more self-documenting.
 */

namespace GameConstants {
    
    // Graphics and Color Constants
    constexpr int COLOR_MAX = 255;
    constexpr int ALPHA_MAX = 255;
    constexpr int RGB_COMPONENTS = 3;
    constexpr int RGBA_COMPONENTS = 4;
    
    // Game Logic Constants  
    constexpr int PLAYER_COUNT = 4;
    constexpr int DIRECTION_COUNT = 4;
    constexpr int MAX_POWER_LEVEL = 3;
    
    // Tile and Grid System
    constexpr int TILE_SIZE = 8;
    constexpr int GRID_SIZE = 10;
    constexpr int ANIMATION_FRAMES = 8;
    
    // Timing and Physics
    constexpr int DEFAULT_SPEED = 20;
    constexpr int TIMER_INTERVAL = 20;
    
    // Bomb System
    constexpr int MAX_BOMBS = 5;
    constexpr int BLAST_RADIUS = 25;
    
    // UI Layout
    constexpr int MENU_WIDTH = 150;
    constexpr int MENU_HEIGHT = 200;
    constexpr int UI_ELEMENT_WIDTH = 150;
    constexpr int UI_ELEMENT_HEIGHT = 200;
    
    // Scoring and Game Balance
    constexpr int SCORE_INCREMENT = 100;
    constexpr int DISTANCE_THRESHOLD = 100;
    constexpr int PERCENT_MAX = 100;
    
    // Audio System Constants
    constexpr float DEFAULT_LISTENER_X = 400.0f;           // Default audio listener X position
    constexpr float DEFAULT_LISTENER_Y = 300.0f;           // Default audio listener Y position  
    constexpr float STEREO_PAN_RANGE = 400.0f;             // Stereo effect range in pixels
    
    // AI and Proximity Detection
    constexpr float EXPLOSION_PROXIMITY_THRESHOLD = 30.0f;  // Distance to consider enemy in explosion range
    constexpr float TILE_COLLISION_TOLERANCE = 20.0f;       // Half tile size for collision detection
    constexpr float AI_TARGET_DISTANCE = 100.0f;            // AI target acquisition distance
    constexpr float AI_PATHFINDING_THRESHOLD = 40.0f;       // AI pathfinding distance threshold
    
    // Common Mathematical Constants
    constexpr int DECIMAL_BASE = 10;
    constexpr int BITS_PER_BYTE = 8;
    
} // namespace GameConstants