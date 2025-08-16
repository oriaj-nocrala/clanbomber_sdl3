# Audio-related Bug Report

This file describes a bug that was found while testing the audio migration.

## Game not returning to menu after victory

**Description:**

The game does not return to the main menu after a player wins a match. The victory screen is displayed, but the game gets stuck there.

**File:** `src/GameplayScreen.cpp`

**Line:** `423`

**Code:**
```cpp
    } else {
        game_over_timer += deltaTime;
        // Return to menu after 8 seconds (more time to enjoy victory)
        if (game_over_timer > 8.0f) {
            // TODO: Transition back to menu
            SDL_Log("Game over timer expired, should return to menu");
        }
    }
```

**Problem:**

The code to transition back to the menu is commented out with a `TODO`. This is why the game gets stuck on the victory screen.

**Suggested Fix:**

Implement the transition to the main menu when the `game_over_timer` expires. This can be done by changing the game state to `GameState::MAIN_MENU`.
