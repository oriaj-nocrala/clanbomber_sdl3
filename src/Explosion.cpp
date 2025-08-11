#include "Explosion.h"
#include "Bomber.h"
#include "Map.h"
#include "MapTile.h"
#include "Resources.h"
#include "Bomb.h"
#include "Timer.h"
#include <SDL3/SDL_timer.h>

Explosion::Explosion(int _x, int _y, int _power, Bomber* _owner, ClanBomberApplication* app) : GameObject(_x, _y, app) {
    owner = _owner;
    power = _power;
    detonation_period = 0.5f; // seconds - exactly like original
    texture_name = "explosion";

    length_up = length_down = length_left = length_right = 0;

    // Calculate explosion lengths
    for (int i = 1; i <= power; ++i) {
        MapTile* tile = app->map->get_tile(get_map_x(), get_map_y() - i);
        if (!tile || tile->is_blocking()) break;
        length_up = i;
        if (tile->is_burnable()) break;
    }
    for (int i = 1; i <= power; ++i) {
        MapTile* tile = app->map->get_tile(get_map_x(), get_map_y() + i);
        if (!tile || tile->is_blocking()) break;
        length_down = i;
        if (tile->is_burnable()) break;
    }
    for (int i = 1; i <= power; ++i) {
        MapTile* tile = app->map->get_tile(get_map_x() - i, get_map_y());
        if (!tile || tile->is_blocking()) break;
        length_left = i;
        if (tile->is_burnable()) break;
    }
    for (int i = 1; i <= power; ++i) {
        MapTile* tile = app->map->get_tile(get_map_x() + i, get_map_y());
        if (!tile || tile->is_blocking()) break;
        length_right = i;
        if (tile->is_burnable()) break;
    }

    detonate_other_bombs();
}

void Explosion::act(float deltaTime) {
    // Detonate other bombs in chain reaction
    detonate_other_bombs();
    
    // Update explosion timer
    detonation_period -= deltaTime;
    if (detonation_period < 0) {
        delete_me = true;
    }
}

void Explosion::detonate_other_bombs() {
    // Center
    MapTile* tile = app->map->get_tile(get_map_x(), get_map_y());
    if (tile && tile->bomb) {
        tile->bomb->explode_delayed();
    }

    // Rays
    for (int i = 1; i <= length_up; ++i) {
        tile = app->map->get_tile(get_map_x(), get_map_y() - i);
        if (tile && tile->bomb) tile->bomb->explode_delayed();
    }
    for (int i = 1; i <= length_down; ++i) {
        tile = app->map->get_tile(get_map_x(), get_map_y() + i);
        if (tile && tile->bomb) tile->bomb->explode_delayed();
    }
    for (int i = 1; i <= length_left; ++i) {
        tile = app->map->get_tile(get_map_x() - i, get_map_y());
        if (tile && tile->bomb) tile->bomb->explode_delayed();
    }
    for (int i = 1; i <= length_right; ++i) {
        tile = app->map->get_tile(get_map_x() + i, get_map_y());
        if (tile && tile->bomb) tile->bomb->explode_delayed();
    }
}


void Explosion::show() {
    // Original animation sequence based on remaining time
    int anim = 14;
    if (detonation_period < 0.4f) anim = 7;
    if (detonation_period < 0.3f) anim = 0;
    if (detonation_period < 0.2f) anim = 7;
    if (detonation_period < 0.1f) anim = 14;

    // Draw center
    draw_part(x, y, EXPLODE_X + anim);

    // Draw rays
    for (int i = 1; i < length_up; ++i) draw_part(x, y - i * 40, EXPLODE_V + anim);
    for (int i = 1; i < length_down; ++i) draw_part(x, y + i * 40, EXPLODE_V + anim);
    for (int i = 1; i < length_left; ++i) draw_part(x - i * 40, y, EXPLODE_H + anim);
    for (int i = 1; i < length_right; ++i) draw_part(x + i * 40, y, EXPLODE_H + anim);

    // Draw tips
    if (length_up > 0) draw_part(x, y - length_up * 40, EXPLODE_UP + anim);
    if (length_down > 0) draw_part(x, y + length_down * 40, EXPLODE_DOWN + anim);
    if (length_left > 0) draw_part(x - length_left * 40, y, EXPLODE_LEFT + anim);
    if (length_right > 0) draw_part(x + length_right * 40, y, EXPLODE_RIGHT + anim);
}

void Explosion::draw_part(int px, int py, int spr_nr) {
    TextureInfo* tex_info = Resources::get_texture(texture_name);
    if (!tex_info || !tex_info->texture) {
        return;
    }

    SDL_FRect src_rect;
    src_rect.w = 40; // Assuming 40x40 sprites
    src_rect.h = 40;
    
    int sprites_per_row = 21; // The original explosion sheet has 21 columns

    src_rect.x = (spr_nr % sprites_per_row) * src_rect.w;
    src_rect.y = (spr_nr / sprites_per_row) * src_rect.h;

    SDL_FRect dest_rect = { (float)px, (float)py, src_rect.w, src_rect.h };

    SDL_RenderTexture(Resources::get_renderer(), tex_info->texture, &src_rect, &dest_rect);
}

