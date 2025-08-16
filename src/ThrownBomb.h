#ifndef THROWNBOMB_H
#define THROWNBOMB_H

#include "Bomb.h"

class ThrownBomb : public Bomb {
public:
    ThrownBomb(int _x, int _y, int _power, Bomber* _owner, 
               float target_x, float target_y, ClanBomberApplication* app);
    
    void act(float deltaTime) override;
    void show() override;

private:
    float start_x, start_y;
    float target_x, target_y;
    float flight_timer;
    float flight_duration;
    bool is_flying;
    float arc_height;
    
    void calculate_flight_path();
};

#endif