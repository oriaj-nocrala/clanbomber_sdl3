#include "MapTile.h"
#include "MapTile_Ground.h"
#include "MapTile_Wall.h"
#include "MapTile_Box.h"
#include "ClanBomber.h"
#include "Extra.h"
#include <random>

MapTile::MapTile(int x, int y, ClanBomberApplication* app) : GameObject(x, y, app) {
    blocking = false;
    destructible = false;
    bomb = nullptr;
    bomber = nullptr;
}

MapTile::~MapTile() {
}

MapTile* MapTile::create(MAPTILE_TYPE type, int x, int y, ClanBomberApplication* app) {
    switch (type) {
        case GROUND:
            return new MapTile_Ground(x, y, app);
        case WALL:
            return new MapTile_Wall(x, y, app);
        case BOX:
            return new MapTile_Box(x, y, app);
        case NONE:
        default:
            return new MapTile_Ground(x, y, app); // Default to ground
    }
}

void MapTile::act() {
    // Base implementation - do nothing
}

void MapTile::show() {
    GameObject::show();
}

void MapTile::destroy() {
    // Base implementation - do nothing
}

void MapTile::spawn_extra() {
    // Based on original ClanBomber spawn logic with balanced probabilities
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> main_dist(0, 7); // 8 main categories (0-7)
    
    int roll = main_dist(gen);
    Extra::EXTRA_TYPE extra_type;
    
    switch (roll) {
        case 0: // Power-ups (25% chance total)
            extra_type = Extra::FLAME;
            break;
        case 1: // Bomb capacity (12.5% chance)
            extra_type = Extra::BOMB;
            break;
        case 2: // Speed (12.5% chance)
            extra_type = Extra::SPEED;
            break;
        case 3: { // Special abilities (12.5% chance - kick or glove)
            std::uniform_int_distribution<> special_dist(0, 1);
            extra_type = (special_dist(gen) == 0) ? Extra::KICK : Extra::GLOVE;
            break;
        }
        case 4: { // Negative effects (12.5% chance)
            std::uniform_int_distribution<> negative_dist(0, 7); // Increased chance of negative effects
            int neg_roll = negative_dist(gen);
            if (neg_roll == 0 || neg_roll == 1) {
                extra_type = Extra::DISEASE; // Constipation (25% of this case)
            } else if (neg_roll == 2 || neg_roll == 3) {
                extra_type = Extra::VIAGRA; // Sticky bombs (25% of this case)
            } else if (neg_roll == 4 || neg_roll == 5) {
                extra_type = Extra::KOKS; // Uncontrollable speed (25% of this case)
            } else {
                return; // No extra spawned (25% of this case)
            }
            break;
        }
        case 5: // Skate (rare, 6.25% chance)
            if (std::uniform_int_distribution<>(0, 1)(gen) == 0) {
                extra_type = Extra::SKATE;
            } else {
                return; // No extra
            }
            break;
        case 6:
        case 7:
        default:
            // Empty cases (25% chance total) - no extra spawned
            return;
    }
    
    // Create the extra at this tile's position
    Extra* extra = new Extra(get_x(), get_y(), extra_type, app);
    app->objects.push_back(extra);
}