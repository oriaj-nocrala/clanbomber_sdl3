# Missing Sounds Suggestions

This file contains a list of suggested sounds that are currently missing from the game.

*   **Step Sounds:** Bombers should make a sound when they walk. This would improve the game's atmosphere and provide better feedback to the player. A `step.wav` sound could be played in the `Bomber::act` function when the bomber is moving.

*   **Item Pickup Sounds:** Currently, only some of the extras have pickup sounds. A generic `pickup.wav` sound should be added for all the other extras. This would make the game more consistent.

*   **Wall Bump Sound:** When a bomber walks into a wall, a sound effect should be played. This would provide better feedback to the player. A `bump.wav` sound could be played when the `Bomber::move` function returns `false`.

*   **Disease Sounds:** The game has a disease system, but only the "koks" extra has a sound effect. It would be great to have sounds for the other diseases as well. For example, a "cough" sound for the "disease" extra.
