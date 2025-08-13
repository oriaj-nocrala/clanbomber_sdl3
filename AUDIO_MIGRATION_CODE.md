# Audio Migration Code

This file contains the details of the code changes performed to migrate the audio system from the legacy SDL2_mixer to the modern `AudioMixer`.

## 1. `src/Bomb.cpp`

*   **Removed:** `Audio::play(explode_sound);`
*   **Replaced with:** `AudioMixer::play_sound_3d("explode", bomb_pos, 600.0f);`
*   **Removed:** `#include "Audio.h"` and `#include "Resources.h"`

## 2. `src/Bomber.cpp`

*   **Removed:** `Audio::play(putbomb_sound);`
*   **Replaced with:** `AudioMixer::play_sound_3d("putbomb", bomber_pos, 400.0f);`
*   **Removed:** `#include "Audio.h"` and `#include "Resources.h"`

## 3. `src/Resources.cpp`

*   Removed the `sounds` map.
*   Removed the `load_sound` function.
*   Removed the `get_sound` function.
*   Modified the `init` function to only load sounds into the `AudioMixer`.
*   Modified the `shutdown` function to not try to free the old `sounds` map.

## 4. `src/Resources.h`

*   Removed the `Sound` struct.
*   Removed the declarations for `load_sound` and `get_sound`.

## 5. `src/Game.cpp`

*   Removed `Audio::init()` and `Audio::shutdown()` calls.
*   Removed `#include "Audio.h"`.

## 6. `CMakeLists.txt`

*   Removed `src/Audio.cpp` from the list of source files.

## 7. `src/Audio.h` and `src/Audio.cpp`

*   Renamed to `src/Audio.h.bak` and `src/Audio.cpp.bak` as a backup.
