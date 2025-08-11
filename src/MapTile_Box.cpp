#include "MapTile_Box.h"
#include "ClanBomber.h"
#include "Resources.h"
#include "AudioMixer.h"
#include "Extra.h"
#include "Timer.h"
#include <random>

MapTile_Box::MapTile_Box(int _x, int _y, ClanBomberApplication* _app) : MapTile(_x, _y, _app) {
    texture_name = "maptiles";
    sprite_nr = 10; // Box tile
    destroyed = false;
    destroy_animation = 0.0f;
    blocking = true;
    destructible = true;
}

MapTile_Box::~MapTile_Box() {
}

void MapTile_Box::act() {
    if (destroyed) {
        destroy_animation += Timer::time_elapsed();
        if (destroy_animation > 0.5f) {
            delete_me = true;
        }
    }
}

void MapTile_Box::show() {
    if (!destroyed) {
        MapTile::show();
    }
    // When destroyed, don't render anything (becomes passable ground)
}

void MapTile_Box::destroy() {
    if (!destroyed) {
        destroyed = true;
        blocking = false;
        destroy_animation = 0.0f;
        
        // Play destruction sound with 3D positioning
        AudioPosition box_pos(get_x(), get_y(), 0.0f);
        AudioMixer::play_sound_3d("break", box_pos, 500.0f);
        
        // 30% chance to drop a power-up
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> drop_chance(0.0, 1.0);
        
        if (drop_chance(gen) < 0.3f) {
            // Choose random power-up type
            std::uniform_int_distribution<> type_dist(0, 8);
            Extra::EXTRA_TYPE extra_type = static_cast<Extra::EXTRA_TYPE>(type_dist(gen));
            
            // Create power-up at this position
            Extra* extra = new Extra(get_x(), get_y(), extra_type, app);
            app->objects.push_back(extra);
        }
        
        // TODO: Add particle effects for destruction
    }
}