#include "ThrownBomb.h"
#include "Timer.h"
#include "Resources.h"
#include "MapTile.h"
#include "Map.h"
#include "CoordinateSystem.h"
#include <cmath>

ThrownBomb::ThrownBomb(int _x, int _y, int _power, Bomber* _owner, 
                       float _target_x, float _target_y, GameContext* context) 
    : Bomb(_x, _y, _power, _owner, context) {
    
    start_x = x;
    start_y = y;
    target_x = _target_x;
    target_y = _target_y;
    
    flight_timer = 0.0f;
    is_flying = true;
    arc_height = 30.0f; // Pixels high for the arc
    
    calculate_flight_path();
    
    SDL_Log("ThrownBomb created: from (%.1f,%.1f) to (%.1f,%.1f), duration=%.2fs", 
            start_x, start_y, target_x, target_y, flight_duration);
}

void ThrownBomb::calculate_flight_path() {
    // Calculate flight duration based on distance
    float distance = sqrt((target_x - start_x) * (target_x - start_x) + 
                         (target_y - start_y) * (target_y - start_y));
    
    // Base flight time on distance (roughly 200 pixels per second)
    flight_duration = std::max(0.5f, distance / 200.0f);
    flight_duration = std::min(2.0f, flight_duration); // Cap at 2 seconds
}

void ThrownBomb::act(float deltaTime) {
    if (is_flying) {
        flight_timer += deltaTime;
        float progress = flight_timer / flight_duration;
        
        if (progress >= 1.0f) {
            // Landing
            is_flying = false;
            x = target_x;
            y = target_y;
            
            // Snap to grid using CoordinateSystem - snap to tile corner first, then center
            GridCoord grid = CoordinateSystem::pixel_to_grid(PixelCoord(x, y));
            PixelCoord center = CoordinateSystem::grid_to_pixel(grid);
            x = static_cast<int>(center.pixel_x);
            y = static_cast<int>(center.pixel_y);
            
            // Update map tile reference using new architecture
            remove_bomb_from_tile(this);  // Remove from old position
            set_bomb_on_tile(this);       // Set at new position
            
            SDL_Log("ThrownBomb landed at grid (%d,%d)", get_map_x(), get_map_y());
        } else {
            // Flying animation - parabolic arc
            float ease_progress = progress; // Linear for now, could add easing
            
            // Linear interpolation for X and Y
            x = start_x + (target_x - start_x) * ease_progress;
            y = start_y + (target_y - start_y) * ease_progress;
            
            // Add parabolic arc for height effect (visual only, doesn't affect collision)
            // Arc peaks at 50% of flight
            float arc_progress = 4.0f * progress * (1.0f - progress); // Parabola: peaks at 0.5
            float visual_offset = arc_height * arc_progress;
            
            // Note: visual_offset will be used in show() for rendering
        }
    } else {
        // Normal bomb behavior once landed
        Bomb::act(deltaTime);
    }
}

void ThrownBomb::show() {
    if (is_flying) {
        // Calculate current arc offset for visual effect
        float progress = flight_timer / flight_duration;
        float arc_progress = 4.0f * progress * (1.0f - progress); // Parabola
        float visual_offset = arc_height * arc_progress;
        
        // Temporarily offset Y position for rendering
        float original_y = y;
        y -= visual_offset; // Move up for arc effect
        
        // Add slight scaling effect when high in the air
        float scale = 1.0f + (visual_offset / arc_height) * 0.3f; // Up to 30% bigger at peak
        
        // For now, just render at offset position (scaling would require more complex rendering)
        Bomb::show();
        
        // Restore original position
        y = original_y;
        
        // TODO: Add shadow effect on ground to show where bomb will land
    } else {
        // Normal bomb rendering when landed
        Bomb::show();
    }
}