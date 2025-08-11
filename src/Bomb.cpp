#include "Bomb.h"
#include "Explosion.h"
#include "ClanBomber.h"
#include "Timer.h"
#include "GameConfig.h"
#include "Map.h"
#include "MapTile.h"
#include "Audio.h"
#include "AudioMixer.h"
#include "Resources.h"
#include "Bomber.h"

Bomb::Bomb(int _x, int _y, int _power, Bomber* _owner, ClanBomberApplication* app) : GameObject(_x, _y, app) {
    texture_name = "bombs";
    power = _power;
    owner = _owner;
    countdown = GameConfig::get_bomb_countdown() / 1000.0f;
    x = ((int)x + 20) / 40 * 40;
    y = ((int)y + 20) / 40 * 40;

    // Animation and color setup
    anim_timer = 0.0f;
    base_sprite = owner->get_color() * 4; // 4 frames per color
    sprite_nr = base_sprite;

    MapTile* tile = app->map->get_tile(get_map_x(), get_map_y());
    if (tile) {
        tile->bomb = this;
    }
}

Bomb::~Bomb() {
    MapTile* tile = app->map->get_tile(get_map_x(), get_map_y());
    if (tile && tile->bomb == this) {
        tile->bomb = nullptr;
    }
}

void Bomb::act(float deltaTime) {
    // Animation logic based on original game
    anim_timer += deltaTime;
    const int num_animation_frames = 4;
    const float animation_speed = 10.0f;
    int current_frame = static_cast<int>(anim_timer * animation_speed) % num_animation_frames;
    sprite_nr = base_sprite + current_frame;

    // Move the bomb if it has been kicked
    if (cur_dir != DIR_NONE) {
        if (!move(deltaTime)) { // move returns false if the bomb is blocked
            stop(); 
        }
    }

    // Countdown to explosion
    countdown -= deltaTime;
    if (countdown <= 0) {
        explode();
    }
}

void Bomb::explode() {
    if (delete_me) return;

    delete_me = true;
    
    // Play explosion sound with 3D positioning
    AudioPosition bomb_pos(x, y, 0.0f);
    if (!AudioMixer::play_sound_3d("explode", bomb_pos, 600.0f)) {
        // Fallback to old system
        Sound* explode_sound = Resources::get_sound("explode");
        if (explode_sound) {
            Audio::play(explode_sound);
        }
    }
    
    // Create explosion
    app->objects.push_back(new Explosion(x, y, power, owner, app));
}

void Bomb::explode_delayed() {
    float delay = GameConfig::get_bomb_delay() / 100.0f;
    if (countdown > delay) {
        countdown = delay;
    }
}

void Bomb::kick(Direction dir) {
    // Can only kick a stationary bomb
    if (cur_dir == DIR_NONE) {
        MapTile* tile = get_tile();
        if (tile && tile->bomb == this) {
            tile->bomb = nullptr;
        }
        cur_dir = dir;
        speed = 240; // Set a speed for the kicked bomb
    }
}

void Bomb::stop() {
    cur_dir = DIR_NONE;
    snap(); // Align to grid
    MapTile* tile = get_tile();
    if (tile) {
        tile->bomb = this;
    }
}