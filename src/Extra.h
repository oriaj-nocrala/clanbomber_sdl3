#ifndef EXTRA_H
#define EXTRA_H

#include "GameObject.h"

class Extra : public GameObject {
public:
    enum EXTRA_TYPE {
        BOMB = 0,         // Increase bomb count
        FLAME = 1,        // Increase explosion range
        SPEED = 2,        // Increase movement speed
        KICK = 3,         // Allow bomb kicking
        GLOVE = 4,        // Allow bomb throwing
        SKATE = 5,        // Ice skates (slide on ice)
        DISEASE = 6,      // Negative effect - constipation
        KOKS = 7,         // Negative effect - makes you fast but uncontrollable
        VIAGRA = 8        // Negative effect - makes bombs stick to you
    };

    Extra(int _x, int _y, EXTRA_TYPE _type, ClanBomberApplication* app);
    ~Extra();

    void act(float deltaTime) override;
    void show() override;
    
    ObjectType get_type() const override { return EXTRA; }
    EXTRA_TYPE get_extra_type() const { return extra_type; }
    
    bool is_collected() const { return collected; }
    void collect();

private:
    void apply_effect_to_bomber(class Bomber* bomber);
    EXTRA_TYPE extra_type;
    bool collected;
    float collect_animation;
    float bounce_timer;
    float bounce_offset;
};

#endif