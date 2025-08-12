#ifndef CORPSEPART_H
#define CORPSEPART_H

#include "GameObject.h"

class CorpsePart : public GameObject {
public:
    CorpsePart(int _x, int _y, int part_type, float velocity_x, float velocity_y, ClanBomberApplication* app);
    ~CorpsePart();

    void act(float deltaTime) override;
    void show() override;
    
    ObjectType get_type() const override { return CORPSE_PART; }

private:
    float velocity_x, velocity_y;
    float gravity;
    float bounce_factor;
    float lifetime;
    float max_lifetime;
    int part_sprite;
    bool has_bounced;
    float rotation;
    float angular_velocity;
};

#endif