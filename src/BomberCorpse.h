#ifndef BOMBERCORPSE_H
#define BOMBERCORPSE_H

#include "GameObject.h"
#include "Bomber.h"

class GameContext;

class BomberCorpse : public GameObject {
public:
    BomberCorpse(int _x, int _y, Bomber::COLOR bomber_color, GameContext* context);
    ~BomberCorpse();

    void act(float deltaTime) override;
    void show() override;
    
    ObjectType get_type() const override { return BOMBER_CORPSE; }
    
    void explode(); // Called when explosion hits the corpse
    bool is_exploded() const { return exploded; }

private:
    Bomber::COLOR color;
    bool exploded;
    float death_animation_timer;
    float gore_explosion_timer;
    bool gore_created;
    
    void create_gore_explosion();
};

#endif