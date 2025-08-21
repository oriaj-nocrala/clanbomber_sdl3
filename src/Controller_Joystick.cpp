#include "Controller_Joystick.h"
#include <SDL3/SDL_log.h>

// Static member definitions
bool Controller_Joystick::joystick_system_initialized = false;
SDL_Joystick* Controller_Joystick::connected_joysticks[8] = {nullptr};

// ===== CONSTRUCTOR / DESTRUCTOR =====

Controller_Joystick::Controller_Joystick(int joystick_index) 
    : Controller(), joystick_index(joystick_index), gamepad(nullptr), joystick(nullptr), instance_id(-1), 
      haptic_device(nullptr), haptic_effect_id(-1) {
    
    c_type = static_cast<CONTROLLER_TYPE>(JOYSTICK_1 + joystick_index);
    
    // Reset input state
    reset();
    
    // Initialize joystick system if not done yet
    if (!joystick_system_initialized) {
        initialize_joystick_system();
    }
    
    // Try to connect to the joystick
    initialize_joystick();
    
    // Initialize haptic feedback
    initialize_haptic();
    
    SDL_Log("Controller_Joystick: Created joystick controller %d", joystick_index);
}

Controller_Joystick::~Controller_Joystick() {
    cleanup_haptic();
    cleanup_joystick();
}

// ===== CONTROLLER INTERFACE =====

void Controller_Joystick::update() {
    if (!active || !is_joystick_connected()) {
        return;
    }
    
    update_input_state();
    
    // SDL3 handles gamepad rumble automatically, no manual update needed
}

void Controller_Joystick::reset() {
    left_pressed = false;
    right_pressed = false;
    up_pressed = false;
    down_pressed = false;
    bomb_pressed = false;
}

bool Controller_Joystick::is_left() {
    if (!active || !is_joystick_connected()) return false;
    return reverse ? right_pressed : left_pressed;
}

bool Controller_Joystick::is_right() {
    if (!active || !is_joystick_connected()) return false;
    return reverse ? left_pressed : right_pressed;
}

bool Controller_Joystick::is_up() {
    if (!active || !is_joystick_connected()) return false;
    return reverse ? down_pressed : up_pressed;
}

bool Controller_Joystick::is_down() {
    if (!active || !is_joystick_connected()) return false;
    return reverse ? up_pressed : down_pressed;
}

bool Controller_Joystick::is_bomb() {
    if (!active || !is_joystick_connected()) return false;
    
    switch (bomb_mode) {
        case ALWAYS:
            return true;
        case NEVER:
            return false;
        case NORMAL:
        default:
            return bomb_pressed;
    }
}

// ===== STATIC JOYSTICK SYSTEM MANAGEMENT =====

void Controller_Joystick::initialize_joystick_system() {
    if (joystick_system_initialized) {
        return;
    }
    
    // Initialize SDL gamepad, joystick and haptic subsystems
    if (SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) < 0) {
        SDL_Log("Controller_Joystick: Failed to initialize SDL gamepad/joystick/haptic subsystems: %s", SDL_GetError());
        return;
    }
    
    // Enable joystick events (SDL3 uses true instead of SDL_TRUE)
    SDL_SetJoystickEventsEnabled(true);
    
    joystick_system_initialized = true;
    
    SDL_Log("Controller_Joystick: Joystick system initialized");
    
    // Get list of joysticks (SDL3 changed API)
    int num_joysticks;
    SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&num_joysticks);
    SDL_Log("Controller_Joystick: Found %d joysticks", num_joysticks);
    
    // Log connected joysticks
    if (joystick_ids) {
        for (int i = 0; i < num_joysticks && i < 8; i++) {
            const char* name = SDL_GetJoystickNameForID(joystick_ids[i]);
            SDL_Log("Controller_Joystick: Joystick %d (ID %d): %s", i, joystick_ids[i], name ? name : "Unknown");
        }
        SDL_free(joystick_ids);
    }
}

void Controller_Joystick::shutdown_joystick_system() {
    if (!joystick_system_initialized) {
        return;
    }
    
    // Close all open joysticks
    for (int i = 0; i < 8; i++) {
        if (connected_joysticks[i]) {
            SDL_CloseJoystick(connected_joysticks[i]);
            connected_joysticks[i] = nullptr;
        }
    }
    
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
    joystick_system_initialized = false;
    
    SDL_Log("Controller_Joystick: Joystick system shutdown");
}

void Controller_Joystick::update_all_joysticks() {
    if (!joystick_system_initialized) {
        return;
    }
    
    // SDL3 handles joystick events automatically through the event system
    // This method is kept for compatibility but doesn't need to do anything special
}

int Controller_Joystick::get_joystick_count() {
    if (!joystick_system_initialized) {
        return 0;
    }
    
    int num_joysticks;
    SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&num_joysticks);
    if (joystick_ids) {
        SDL_free(joystick_ids);
        return num_joysticks;
    }
    return 0;
}

// ===== PRIVATE METHODS =====

bool Controller_Joystick::initialize_joystick() {
    // Use SDL3 Gamepad API like your working example
    int num_joysticks = 0;
    
    // Get all joystick IDs
    SDL_JoystickID* joystick_ids = SDL_GetJoysticks(&num_joysticks);
    if (!joystick_ids) {
        SDL_Log("Controller_Joystick: Failed to get joystick IDs: %s", SDL_GetError());
        return false;
    }
    
    SDL_Log("Controller_Joystick: Found %d total joysticks", num_joysticks);
    
    // Find the joystick_index-th gamepad 
    int gamepad_count = 0;
    for (int i = 0; i < num_joysticks; ++i) {
        if (SDL_IsGamepad(joystick_ids[i])) {
            if (gamepad_count == joystick_index) {
                // Found our target gamepad
                gamepad = SDL_OpenGamepad(joystick_ids[i]);
                if (gamepad) {
                    const char* name = SDL_GetGamepadName(gamepad);
                    SDL_Log("Controller_Joystick: Opened Gamepad %d: %s", joystick_index, name ? name : "Unknown");
                    
                    // Get the underlying joystick (like your example)
                    joystick = SDL_GetGamepadJoystick(gamepad);
                    if (joystick) {
                        instance_id = SDL_GetJoystickID(joystick);
                        connected_joysticks[joystick_index] = joystick;
                        
                        int num_buttons = SDL_GetNumJoystickButtons(joystick);
                        int num_axes = SDL_GetNumJoystickAxes(joystick);
                        int num_hats = SDL_GetNumJoystickHats(joystick);
                        SDL_Log("Controller_Joystick: Underlying joystick - Buttons: %d, Axes: %d, Hats: %d", num_buttons, num_axes, num_hats);
                        
                        SDL_free(joystick_ids);
                        return true;
                    } else {
                        SDL_Log("Controller_Joystick: Failed to get underlying joystick from Gamepad");
                        SDL_CloseGamepad(gamepad);
                        gamepad = nullptr;
                        SDL_free(joystick_ids);
                        return false;
                    }
                } else {
                    SDL_Log("Controller_Joystick: Failed to open Gamepad for ID %d: %s", joystick_ids[i], SDL_GetError());
                    SDL_free(joystick_ids);
                    return false;
                }
            }
            gamepad_count++;
        }
    }
    
    SDL_Log("Controller_Joystick: Gamepad index %d not found (found %d gamepads total)", 
            joystick_index, gamepad_count);
    SDL_free(joystick_ids);
    return false;
}

void Controller_Joystick::cleanup_joystick() {
    if (gamepad) {
        SDL_CloseGamepad(gamepad);
        gamepad = nullptr;
    }
    
    if (joystick) {
        // Note: joystick is closed automatically when Gamepad is closed
        connected_joysticks[joystick_index] = nullptr;
        joystick = nullptr;
        instance_id = -1;
    }
}

bool Controller_Joystick::is_joystick_connected() const {
    return joystick && SDL_JoystickConnected(joystick);
}

void Controller_Joystick::update_input_state() {
    if (!joystick) {
        return;
    }
    
    // Update directional input (analog stick OR d-pad)
    left_pressed = get_analog_left() || get_dpad_left();
    right_pressed = get_analog_right() || get_dpad_right();
    up_pressed = get_analog_up() || get_dpad_up();
    down_pressed = get_analog_down() || get_dpad_down();
    
    // Update bomb button
    bomb_pressed = get_button_bomb();
}

// ===== ANALOG STICK INPUT =====

bool Controller_Joystick::get_analog_left() const {
    if (!joystick || button_map.axis_horizontal >= SDL_GetNumJoystickAxes(joystick)) {
        return false;
    }
    
    Sint16 axis_value = SDL_GetJoystickAxis(joystick, button_map.axis_horizontal);
    return axis_value < -AXIS_THRESHOLD;
}

bool Controller_Joystick::get_analog_right() const {
    if (!joystick || button_map.axis_horizontal >= SDL_GetNumJoystickAxes(joystick)) {
        return false;
    }
    
    Sint16 axis_value = SDL_GetJoystickAxis(joystick, button_map.axis_horizontal);
    return axis_value > AXIS_THRESHOLD;
}

bool Controller_Joystick::get_analog_up() const {
    if (!joystick || button_map.axis_vertical >= SDL_GetNumJoystickAxes(joystick)) {
        return false;
    }
    
    Sint16 axis_value = SDL_GetJoystickAxis(joystick, button_map.axis_vertical);
    return axis_value < -AXIS_THRESHOLD;
}

bool Controller_Joystick::get_analog_down() const {
    if (!joystick || button_map.axis_vertical >= SDL_GetNumJoystickAxes(joystick)) {
        return false;
    }
    
    Sint16 axis_value = SDL_GetJoystickAxis(joystick, button_map.axis_vertical);
    return axis_value > AXIS_THRESHOLD;
}

// ===== D-PAD INPUT =====

bool Controller_Joystick::get_dpad_left() const {
    if (!joystick || button_map.hat_index >= SDL_GetNumJoystickHats(joystick)) {
        return false;
    }
    
    Uint8 hat_value = SDL_GetJoystickHat(joystick, button_map.hat_index);
    return (hat_value & SDL_HAT_LEFT) != 0;
}

bool Controller_Joystick::get_dpad_right() const {
    if (!joystick || button_map.hat_index >= SDL_GetNumJoystickHats(joystick)) {
        return false;
    }
    
    Uint8 hat_value = SDL_GetJoystickHat(joystick, button_map.hat_index);
    return (hat_value & SDL_HAT_RIGHT) != 0;
}

bool Controller_Joystick::get_dpad_up() const {
    if (!joystick || button_map.hat_index >= SDL_GetNumJoystickHats(joystick)) {
        return false;
    }
    
    Uint8 hat_value = SDL_GetJoystickHat(joystick, button_map.hat_index);
    return (hat_value & SDL_HAT_UP) != 0;
}

bool Controller_Joystick::get_dpad_down() const {
    if (!joystick || button_map.hat_index >= SDL_GetNumJoystickHats(joystick)) {
        return false;
    }
    
    Uint8 hat_value = SDL_GetJoystickHat(joystick, button_map.hat_index);
    return (hat_value & SDL_HAT_DOWN) != 0;
}

// ===== BUTTON INPUT =====

bool Controller_Joystick::get_button_bomb() const {
    if (!joystick) {
        return false;
    }
    
    // Check primary bomb button
    if (button_map.button_bomb < SDL_GetNumJoystickButtons(joystick)) {
        if (SDL_GetJoystickButton(joystick, button_map.button_bomb)) {
            return true;
        }
    }
    
    // Check alternative bomb button
    if (button_map.button_alt_bomb < SDL_GetNumJoystickButtons(joystick)) {
        if (SDL_GetJoystickButton(joystick, button_map.button_alt_bomb)) {
            return true;
        }
    }
    
    return false;
}

// ===== HAPTIC FEEDBACK SYSTEM =====

void Controller_Joystick::initialize_haptic() {
    if (!gamepad) {
        SDL_Log("Controller_Joystick: No gamepad available for rumble initialization");
        return;
    }
    
    const char* name = SDL_GetGamepadName(gamepad);
    SDL_Log("Controller_Joystick: Checking rumble support for '%s'", name ? name : "Unknown");
    
    // SDL3 SOLUTION: Use SDL_RumbleGamepad directly instead of haptic system!
    // This is much simpler and works better with gamepads
    SDL_Log("Controller_Joystick: Testing SDL_RumbleGamepad (SDL3 native approach)...");
    
    // Test rumble: mid intensity on both motors for 200ms
    if (SDL_RumbleGamepad(gamepad, 32000, 32000, 200)) {
        SDL_Log("Controller_Joystick: ✅ SDL_RumbleGamepad test successful!");
        // Set flag to indicate gamepad rumble is working
        haptic_device = (SDL_Haptic*)1; // Use as boolean flag (non-null = working)
    } else {
        SDL_Log("Controller_Joystick: ❌ SDL_RumbleGamepad test failed: %s", SDL_GetError());
        haptic_device = nullptr;
    }
}

void Controller_Joystick::cleanup_haptic() {
    if (haptic_device && gamepad) {
        // Stop any ongoing rumble
        SDL_RumbleGamepad(gamepad, 0, 0, 0); // Stop rumble
        haptic_device = nullptr;
        SDL_Log("Controller_Joystick: Gamepad rumble stopped for joystick %d", joystick_index);
    }
}

void Controller_Joystick::apply_vibration(float intensity) {
    if (!haptic_device || !gamepad || intensity <= 0.0f) {
        return;
    }
    
    // Clamp intensity to valid range
    intensity = std::fmax(0.0f, std::fmin(1.0f, intensity));
    
    // INTELLIGENT DUAL-MOTOR PHYSICS!
    // Low freq (left motor): Deep bass-like rumble for powerful explosions
    // High freq (right motor): Sharp high-frequency buzz for detailed effects
    
    // Calculate motor intensities based on physics
    Uint16 low_freq, high_freq;
    
    if (intensity >= 0.8f) {
        // MASSIVE EXPLOSION: Both motors at high intensity
        low_freq = (Uint16)(intensity * 50000);  // Strong low rumble
        high_freq = (Uint16)(intensity * 35000); // Sharp high buzz
    } else if (intensity >= 0.5f) {
        // MEDIUM EXPLOSION: Balanced mix, more low freq
        low_freq = (Uint16)(intensity * 45000);
        high_freq = (Uint16)(intensity * 20000);
    } else if (intensity >= 0.2f) {
        // DISTANT EXPLOSION: Mostly low freq with subtle high
        low_freq = (Uint16)(intensity * 30000);
        high_freq = (Uint16)(intensity * 8000);
    } else {
        // VERY DISTANT: Only subtle low frequency
        low_freq = (Uint16)(intensity * 15000);
        high_freq = (Uint16)(intensity * 2000);
    }
    
    // Apply rumble using SDL3's dual-motor system
    if (!SDL_RumbleGamepad(gamepad, low_freq, high_freq, 100)) { // 100ms duration per frame
        SDL_Log("Controller_Joystick: Failed to rumble gamepad: %s", SDL_GetError());
    }
}

float Controller_Joystick::calculate_explosion_intensity(float explosion_x, float explosion_y, float explosion_power, 
                                                       float bomber_x, float bomber_y, bool bomber_died) const {
    // Physics-based calculation using inverse square law with realistic game scaling
    
    // Calculate distance between explosion and bomber
    float dx = explosion_x - bomber_x;
    float dy = explosion_y - bomber_y;
    float distance = std::sqrt(dx * dx + dy * dy);
    
    // Minimum distance to avoid division by zero (1 pixel minimum)
    const float MIN_DISTANCE = 1.0f;
    distance = std::fmax(distance, MIN_DISTANCE);
    
    // BOMBER MOVEMENT PHYSICS: Bomber moves freely at 60 px/sec, not tile-locked
    // Screen is ~800x600, so bomber can be anywhere. Explosions should feel much further!
    
    float intensity;
    if (distance <= 30.0f) {
        // POINT BLANK: Maximum effect (0.8-1.0)
        intensity = 1.0f - (distance / 60.0f); // 0px = 1.0, 30px = 0.5, clamped to 0.8+ 
        intensity = std::fmax(0.8f, intensity);
    } else if (distance <= 80.0f) {
        // VERY CLOSE: Strong vibration (0.5-0.8) - within ~2 tiles but bomber moves freely
        float t = (distance - 30.0f) / 50.0f; // 0 to 1
        intensity = 0.8f - (0.3f * t); // 0.8 down to 0.5
    } else if (distance <= 150.0f) {
        // CLOSE: Good vibration (0.25-0.5) - easily felt across screen sections
        float t = (distance - 80.0f) / 70.0f; // 0 to 1
        intensity = 0.5f - (0.25f * t); // 0.5 down to 0.25
    } else if (distance <= 250.0f) {
        // MEDIUM: Moderate vibration (0.1-0.25) - halfway across screen
        float t = (distance - 150.0f) / 100.0f; // 0 to 1
        intensity = 0.25f - (0.15f * t); // 0.25 down to 0.1
    } else if (distance <= 400.0f) {
        // DISTANT: Weak but felt (0.05-0.1) - across most of screen
        float t = (distance - 250.0f) / 150.0f; // 0 to 1
        intensity = 0.1f - (0.05f * t); // 0.1 down to 0.05
    } else if (distance <= 600.0f) {
        // VERY FAR: Barely felt (0.02-0.05) - full screen diagonal
        float t = (distance - 400.0f) / 200.0f; // 0 to 1
        intensity = 0.05f - (0.03f * t); // 0.05 down to 0.02
    } else {
        // OFF SCREEN: Almost nothing
        intensity = 0.01f * std::fmax(0.0f, (800.0f - distance) / 200.0f);
    }
    
    // Apply explosion power multiplier
    intensity *= explosion_power;
    
    // Special case: If this bomber died, maximum vibration
    if (bomber_died) {
        intensity = 1.0f;
        SDL_Log("HAPTIC: Bomber died! Maximum vibration intensity = %.3f", intensity);
    } else {
        SDL_Log("HAPTIC: Explosion at (%.1f,%.1f) power=%.1f, bomber at (%.1f,%.1f), distance=%.1f px, intensity=%.3f", 
                explosion_x, explosion_y, explosion_power, bomber_x, bomber_y, distance, intensity);
    }
    
    // Clamp to valid range
    return std::fmax(0.0f, std::fmin(1.0f, intensity));
}

void Controller_Joystick::trigger_explosion_vibration(float explosion_x, float explosion_y, float explosion_power, 
                                                    float bomber_x, float bomber_y, bool bomber_died) {
    if (!haptic_device || !gamepad) {
        SDL_Log("HAPTIC: No gamepad rumble available (explosion at %.1f,%.1f power=%.1f)", 
                explosion_x, explosion_y, explosion_power);
        return;
    }
    
    // Calculate physics-based intensity
    float intensity = calculate_explosion_intensity(explosion_x, explosion_y, explosion_power, 
                                                   bomber_x, bomber_y, bomber_died);
    
    // Skip very weak vibrations to avoid noise - lowered for better reach
    const float MIN_VIBRATION_THRESHOLD = 0.005f;
    if (intensity < MIN_VIBRATION_THRESHOLD && !bomber_died) {
        SDL_Log("HAPTIC: Explosion too weak (%.3f), skipping vibration", intensity);
        return;
    }
    
    // Configure vibration based on scenario
    if (bomber_died) {
        // DEATH VIBRATION: Special dramatic effect with dual-motor choreography!
        vibration_state.active = true;
        vibration_state.intensity = 1.0f;
        vibration_state.duration_left = 1.5f;  // 1.5 seconds
        vibration_state.decay_rate = 0.6f;     // Slower decay for dramatic effect
        
        // IMMEDIATE DEATH SHOCK: Short intense burst on both motors
        SDL_RumbleGamepad(gamepad, 65535, 65535, 150); // MAX intensity for 150ms
        
        SDL_Log("HAPTIC: ☠️ DEATH vibration triggered - intensity=%.3f, duration=%.1fs", 
                vibration_state.intensity, vibration_state.duration_left);
    } else {
        // EXPLOSION VIBRATION: Direct call to SDL_RumbleGamepad (like your SDL2 example)
        Uint16 low_freq, high_freq;
        Uint32 duration;
        
        if (intensity >= 0.8f) {
            // MASSIVE EXPLOSION
            low_freq = (Uint16)(intensity * 50000);
            high_freq = (Uint16)(intensity * 35000);
            duration = 600;
        } else if (intensity >= 0.5f) {
            // MEDIUM EXPLOSION
            low_freq = (Uint16)(intensity * 45000);
            high_freq = (Uint16)(intensity * 20000);
            duration = 400;
        } else if (intensity >= 0.2f) {
            // DISTANT EXPLOSION
            low_freq = (Uint16)(intensity * 30000);
            high_freq = (Uint16)(intensity * 8000);
            duration = 300;
        } else {
            // VERY DISTANT
            low_freq = (Uint16)(intensity * 15000);
            high_freq = (Uint16)(intensity * 2000);
            duration = 200;
        }
        
        SDL_Log("HAPTIC: 💥 Explosion rumble - intensity=%.3f, low=%d, high=%d, duration=%dms", 
                intensity, low_freq, high_freq, duration);
        
        // DIRECT RUMBLE CALL (like SDL2 working example)
        if (!SDL_RumbleGamepad(gamepad, low_freq, high_freq, duration)) {
            SDL_Log("HAPTIC: Failed to rumble gamepad: %s", SDL_GetError());
        } else {
            SDL_Log("HAPTIC: ✅ Explosion rumble successful!");
        }
    }
}

void Controller_Joystick::update_haptic(float deltaTime) {
    if (!vibration_state.active || !haptic_device) {
        return;
    }
    
    // Apply current vibration intensity
    apply_vibration(vibration_state.intensity);
    
    // Update vibration state
    vibration_state.duration_left -= deltaTime;
    
    // Apply decay (energy dissipation like real physics)
    vibration_state.intensity -= vibration_state.decay_rate * deltaTime;
    
    // Stop vibration if time expired or intensity too low
    if (vibration_state.duration_left <= 0.0f || vibration_state.intensity <= 0.0f) {
        vibration_state.active = false;
        vibration_state.intensity = 0.0f;
        SDL_Log("HAPTIC: Vibration stopped");
    }
}

void Controller_Joystick::stop_vibration() {
    vibration_state.active = false;
    vibration_state.intensity = 0.0f;
    vibration_state.duration_left = 0.0f;
    
    if (haptic_device && gamepad) {
        SDL_RumbleGamepad(gamepad, 0, 0, 0); // Stop both motors
    }
    
    SDL_Log("HAPTIC: Dual-motor vibration stopped manually");
}