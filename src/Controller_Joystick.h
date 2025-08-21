#ifndef CONTROLLER_JOYSTICK_H
#define CONTROLLER_JOYSTICK_H

#include "Controller.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gamepad.h>
#include <cmath>

/**
 * Controller_Joystick: SDL3 Joystick/Gamepad controller support
 * 
 * Supports both analog sticks and digital buttons.
 * Compatible with Xbox, PlayStation, and generic gamepads.
 */
class Controller_Joystick : public Controller {
public:
    /**
     * @brief Constructor
     * @param joystick_index Index of the joystick (0-7)
     */
    Controller_Joystick(int joystick_index);
    virtual ~Controller_Joystick();
    
    // Controller interface implementation
    void update() override;
    void reset() override;
    bool is_left() override;
    bool is_right() override;
    bool is_up() override;
    bool is_down() override;
    bool is_bomb() override;

    // Joystick management
    static void initialize_joystick_system();
    static void shutdown_joystick_system();
    static void update_all_joysticks();
    static int get_joystick_count();
    
    // Haptic feedback (vibration) - Physics-based system
    void trigger_explosion_vibration(float explosion_x, float explosion_y, float explosion_power, float bomber_x, float bomber_y, bool bomber_died = false);
    void update_haptic(float deltaTime);
    void stop_vibration();

private:
    int joystick_index;
    SDL_Gamepad* gamepad;                // Use Gamepad API instead (SDL3 naming)
    SDL_Joystick* joystick;              // Obtained from Gamepad
    SDL_JoystickID instance_id;
    
    // Haptic (vibration) support
    SDL_Haptic* haptic_device;
    int haptic_effect_id;
    
    // Physics-based vibration system
    struct VibrationState {
        bool active;
        float intensity;         // Current vibration intensity (0.0 - 1.0)
        float duration_left;     // Time remaining in seconds
        float decay_rate;        // How fast vibration decays per second
        
        VibrationState() : active(false), intensity(0.0f), duration_left(0.0f), decay_rate(1.0f) {}
    } vibration_state;
    
    // Current input state
    bool left_pressed;
    bool right_pressed;
    bool up_pressed;
    bool down_pressed;
    bool bomb_pressed;
    
    // Analog stick thresholds
    static constexpr float ANALOG_THRESHOLD = 0.3f;  // 30% threshold for analog input
    static constexpr Sint16 AXIS_THRESHOLD = 10000;  // SDL axis threshold (-32768 to 32767)
    
    // Button mapping (configurable for different gamepad types)
    struct ButtonMapping {
        int button_bomb = 0;      // A button (Xbox) / X button (PlayStation)
        int button_alt_bomb = 1;  // B button (Xbox) / Circle button (PlayStation)
        
        int axis_horizontal = 0;  // Left stick X-axis
        int axis_vertical = 1;    // Left stick Y-axis
        
        // D-pad mapping
        int hat_index = 0;        // Hat/D-pad index
    } button_map;
    
    // Helper methods
    bool initialize_joystick();
    void cleanup_joystick();
    bool is_joystick_connected() const;
    void update_input_state();
    
    // Haptic helper methods
    void initialize_haptic();
    void cleanup_haptic();
    void apply_vibration(float intensity);
    
    // Physics calculations for realistic vibration
    float calculate_explosion_intensity(float explosion_x, float explosion_y, float explosion_power, 
                                       float bomber_x, float bomber_y, bool bomber_died) const;
    
    // Input processing
    bool get_analog_left() const;
    bool get_analog_right() const;  
    bool get_analog_up() const;
    bool get_analog_down() const;
    bool get_button_bomb() const;
    
    // D-pad processing
    bool get_dpad_left() const;
    bool get_dpad_right() const;
    bool get_dpad_up() const;
    bool get_dpad_down() const;
    
    // Static joystick management
    static bool joystick_system_initialized;
    static SDL_Joystick* connected_joysticks[8];
};

#endif