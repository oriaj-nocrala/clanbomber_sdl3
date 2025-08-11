#include "Bomb.h"
#include "Explosion.h"
#include "ClanBomber.h"
#include "Timer.h"
#include "GameConfig.h"
#include "Map.h"
#include "MapTile.h"
#include "Audio.h"
#include "Resources.h"
#include "Bomber.h"

Bomb::Bomb(int _x, int _y, int _power, Bomber* _owner, ClanBomberApplication* app) : GameObject(_x, _y, app) {
    texture_name = "bombs";
    sprite_nr = 0;
    power = _power;
    owner = _owner;
    countdown = GameConfig::get_bomb_countdown() / 1000.0f;
    x = ((int)x + 20) / 40 * 40;
    y = ((int)y + 20) / 40 * 40;

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
    countdown -= deltaTime;
    if (countdown <= 0) {
        explode();
    }
}

void Bomb::explode() {
    if (delete_me) return;

    delete_me = true;
    Audio::play(Resources::get_sound("explode"));
    app->objects.push_back(new Explosion(x, y, power, owner, app));
}

void Bomb::explode_delayed() {
    float delay = GameConfig::get_bomb_delay() / 100.0f;
    if (countdown > delay) {
        countdown = delay;
    }
}

void Bomb::kick(Direction dir) {
    // TODO: implement bomb kicking
}
