#include "Bomb.h"
#include "Explosion.h"
#include "ClanBomber.h"
#include "Timer.h"
#include "GameConfig.h"
#include "Map.h"
#include "MapTile.h"
#include "AudioMixer.h"
#include "Bomber.h"
#include "GameContext.h"

Bomb::Bomb(int _x, int _y, int _power, Bomber* _owner, GameContext* context) : GameObject(_x, _y, context) {
    texture_name = "bombs";
    power = _power;
    owner = _owner;
    countdown = GameConfig::get_bomb_countdown() / 1000.0f;
    
    // GLOBAL CENTERING FIX: Don't override GameObject's centering system!
    // GameObject constructor already centers at tile center - no need to override
    SDL_Log("💣 BOMB: Using GameObject global centering at (%.1f,%.1f)", x, y);

    // CRITICAL: Set Z-order so bombs render ABOVE tiles
    z = Z_BOMB;  // Bombs should be above tiles (3000 > 0)

    // Animation and color setup
    anim_timer = 0.0f;
    base_sprite = owner->get_color() * 4; // 4 frames per color
    sprite_nr = base_sprite;

    // NEW ARCHITECTURE: Synchronize bomb with both architectures
    set_bomb_on_tile(this);
}

Bomb::~Bomb() {
    // NEW ARCHITECTURE: Remove bomb from both architectures  
    remove_bomb_from_tile(this);
}

void Bomb::act(float deltaTime) {
    // Animation logic calibrated for deltaTime 
    anim_timer += deltaTime;
    const int num_animation_frames = 4;
    const float animation_speed = 3.0f; // Reduced from 10.0f for deltaTime calibration
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
    
    // Decrement bomb count for the owner
    if (owner) {
        owner->dec_current_bombs();
        SDL_Log("Bomb exploded, bomber now has %d/%d bombs", owner->get_current_bombs(), owner->get_max_bombs());
    }
    
    // Play explosion sound with 3D positioning
    AudioPosition bomb_pos(x, y, 0.0f);
    AudioMixer::play_sound_3d("explode", bomb_pos, 600.0f);
    
    // Create explosion using GameContext registration
    Explosion* explosion = new Explosion(x, y, power, owner, get_context());
    get_context()->register_object(explosion);
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
        // NEW ARCHITECTURE: Remove bomb from both architectures when kicked
        remove_bomb_from_tile(this);
        cur_dir = dir;
        speed = 120; // Reduced from 240 for deltaTime calibration - kicked bomb speed
    }
}

void Bomb::stop() {
    cur_dir = DIR_NONE;
    snap(); // Align to grid
    // NEW ARCHITECTURE: Set bomb on both architectures when stopped
    set_bomb_on_tile(this);
}