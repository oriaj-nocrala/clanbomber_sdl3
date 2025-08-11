#include "Map.h"
#include "MapTile_Wall.h"
#include "MapTile_Ground.h"
#include "ClanBomber.h"

Map::Map(ClanBomberApplication* _app) {
    app = _app;
    width = 20; // 800 / 40
    height = 15; // 600 / 40
}

Map::~Map() {
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            delete tiles[i][j];
        }
    }
}

void Map::load() {
    tiles.resize(width);
    for (int i = 0; i < width; ++i) {
        tiles[i].resize(height);
    }

    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            if (i == 0 || j == 0 || i == width - 1 || j == height - 1) {
                tiles[i][j] = new MapTile_Wall(i * 40, j * 40, app);
            } else {
                tiles[i][j] = new MapTile_Ground(i * 40, j * 40, app);
            }
        }
    }
}

void Map::show() {
    for (int i = 0; i < width; ++i) {
        for (int j = 0; j < height; ++j) {
            tiles[i][j]->show();
        }
    }
}

MapTile* Map::get_tile(int tx, int ty) {
    if (tx >= 0 && tx < width && ty >= 0 && ty < height) {
        return tiles[tx][ty];
    }
    return nullptr;
}
