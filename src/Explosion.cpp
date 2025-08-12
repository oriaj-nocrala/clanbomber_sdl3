#include "Explosion.h"
#include "Bomber.h"
#include "BomberCorpse.h"
#include "Map.h"
#include "MapTile.h"
#include "Resources.h"
#include "Bomb.h"
#include "Timer.h"
#include "ParticleSystem.h"
#include <SDL3/SDL_timer.h>

Explosion::Explosion(int _x, int _y, int _power, Bomber* _owner, ClanBomberApplication* app) : GameObject(_x, _y, app) {
    owner = _owner;
    power = _power;
    detonation_period = 0.5f; // seconds - exactly like original
    texture_name = "explosion";
    
    // Create explosion particle effects
    ParticleSystem* explosion_sparks = new ParticleSystem(_x, _y, EXPLOSION_SPARKS, app);
    app->objects.push_back(explosion_sparks);
    
    ParticleSystem* dust_cloud = new ParticleSystem(_x, _y, DUST_CLOUDS, app);
    app->objects.push_back(dust_cloud);

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
    // Detonate other bombs, kill bombers, and explode corpses
    detonate_other_bombs();
    kill_bombers();
    explode_corpses();
    
    // Update explosion timer
    detonation_period -= deltaTime;
    if (detonation_period < 0) {
        delete_me = true;
    }
}

void Explosion::detonate_other_bombs() {
    // Center - destroy destructibles and detonate bombs
    MapTile* tile = app->map->get_tile(get_map_x(), get_map_y());
    if (tile) {
        if (tile->bomb) {
            tile->bomb->explode_delayed();
        }
        if (tile->is_burnable()) {
            tile->destroy();
        }
    }

    // Rays - destroy destructibles and detonate bombs along explosion path
    for (int i = 1; i <= length_up; ++i) {
        tile = app->map->get_tile(get_map_x(), get_map_y() - i);
        if (tile) {
            if (tile->bomb) tile->bomb->explode_delayed();
            if (tile->is_burnable()) {
                tile->destroy();
            }
        }
    }
    for (int i = 1; i <= length_down; ++i) {
        tile = app->map->get_tile(get_map_x(), get_map_y() + i);
        if (tile) {
            if (tile->bomb) tile->bomb->explode_delayed();
            if (tile->is_burnable()) {
                tile->destroy();
            }
        }
    }
    for (int i = 1; i <= length_left; ++i) {
        tile = app->map->get_tile(get_map_x() - i, get_map_y());
        if (tile) {
            if (tile->bomb) tile->bomb->explode_delayed();
            if (tile->is_burnable()) {
                tile->destroy();
            }
        }
    }
    for (int i = 1; i <= length_right; ++i) {
        tile = app->map->get_tile(get_map_x() + i, get_map_y());
        if (tile) {
            if (tile->bomb) tile->bomb->explode_delayed();
            if (tile->is_burnable()) {
                tile->destroy();
            }
        }
    }
}


void Explosion::show() {
    // Improved animation sequence with better timing
    float normalized_time = (0.5f - detonation_period) / 0.5f; // 0.0 to 1.0
    int anim_frame = 0;
    
    if (normalized_time < 0.2f) {
        anim_frame = 0; // Initial expansion
    } else if (normalized_time < 0.5f) {
        anim_frame = 7; // Peak intensity
    } else if (normalized_time < 0.8f) {
        anim_frame = 14; // Sustained burn
    } else {
        anim_frame = 7; // Fade out
    }

    // Add alpha blending for fade effect near end
    if (normalized_time > 0.9f) {
        float alpha = 1.0f - ((normalized_time - 0.9f) / 0.1f);
        SDL_SetTextureAlphaMod(Resources::get_texture(texture_name)->texture, (Uint8)(alpha * 255));
    }

    // Draw center explosion
    draw_part(x, y, EXPLODE_X + anim_frame);

    // Draw explosion rays with proper sprites
    for (int i = 1; i < length_up; ++i) draw_part(x, y - i * 40, EXPLODE_V + anim_frame);
    for (int i = 1; i < length_down; ++i) draw_part(x, y + i * 40, EXPLODE_V + anim_frame);
    for (int i = 1; i < length_left; ++i) draw_part(x - i * 40, y, EXPLODE_H + anim_frame);
    for (int i = 1; i < length_right; ++i) draw_part(x + i * 40, y, EXPLODE_H + anim_frame);

    // Draw explosion tips
    if (length_up > 0) draw_part(x, y - length_up * 40, EXPLODE_UP + anim_frame);
    if (length_down > 0) draw_part(x, y + length_down * 40, EXPLODE_DOWN + anim_frame);
    if (length_left > 0) draw_part(x - length_left * 40, y, EXPLODE_LEFT + anim_frame);
    if (length_right > 0) draw_part(x + length_right * 40, y, EXPLODE_RIGHT + anim_frame);
    
    // Reset alpha for next frame
    SDL_SetTextureAlphaMod(Resources::get_texture(texture_name)->texture, 255);
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

void Explosion::kill_bombers() {
    // Check for bombers in explosion area and kill them
    for (auto& bomber : app->bomber_objects) {
        if (bomber && !bomber->delete_me && !bomber->is_dead()) {
            int bomber_map_x = bomber->get_map_x();
            int bomber_map_y = bomber->get_map_y();
            
            // Check if bomber is in explosion center
            if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y()) {
                bomber->die();
                continue;
            }
            
            // Check if bomber is in explosion rays
            // Up ray
            for (int i = 1; i <= length_up; ++i) {
                if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y() - i) {
                    bomber->die();
                    break;
                }
            }
            // Down ray
            for (int i = 1; i <= length_down; ++i) {
                if (bomber_map_x == get_map_x() && bomber_map_y == get_map_y() + i) {
                    bomber->die();
                    break;
                }
            }
            // Left ray
            for (int i = 1; i <= length_left; ++i) {
                if (bomber_map_x == get_map_x() - i && bomber_map_y == get_map_y()) {
                    bomber->die();
                    break;
                }
            }
            // Right ray
            for (int i = 1; i <= length_right; ++i) {
                if (bomber_map_x == get_map_x() + i && bomber_map_y == get_map_y()) {
                    bomber->die();
                    break;
                }
            }
        }
    }
}

void Explosion::explode_corpses() {
    // Check for corpses in explosion area and make them explode with gore
    for (auto& obj : app->objects) {
        if (obj && obj->get_type() == BOMBER_CORPSE) {
            BomberCorpse* corpse = static_cast<BomberCorpse*>(obj);
            if (!corpse->is_exploded()) {
                int corpse_map_x = corpse->get_map_x();
                int corpse_map_y = corpse->get_map_y();
                
                // Check if corpse is in explosion area (same logic as bombers)
                bool in_explosion = false;
                
                // Center
                if (corpse_map_x == get_map_x() && corpse_map_y == get_map_y()) {
                    in_explosion = true;
                }
                
                // Rays
                if (!in_explosion) {
                    for (int i = 1; i <= length_up && !in_explosion; ++i) {
                        if (corpse_map_x == get_map_x() && corpse_map_y == get_map_y() - i) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_down && !in_explosion; ++i) {
                        if (corpse_map_x == get_map_x() && corpse_map_y == get_map_y() + i) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_left && !in_explosion; ++i) {
                        if (corpse_map_x == get_map_x() - i && corpse_map_y == get_map_y()) {
                            in_explosion = true;
                        }
                    }
                    for (int i = 1; i <= length_right && !in_explosion; ++i) {
                        if (corpse_map_x == get_map_x() + i && corpse_map_y == get_map_y()) {
                            in_explosion = true;
                        }
                    }
                }
                
                if (in_explosion) {
                    corpse->explode(); // This creates the gore explosion!
                }
            }
        }
    }
}

