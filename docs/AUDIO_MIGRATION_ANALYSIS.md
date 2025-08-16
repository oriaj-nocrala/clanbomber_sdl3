# Audio Migration Analysis

This document analyzes the legacy audio calls in the ClanBomber codebase and outlines the plan for migrating them to the modern `AudioMixer` system.

## Task 1: Complete Analysis of Legacy Audio

### Files with Legacy Audio Calls

*   `src/Bomb.cpp`: Contains `Audio::play` for explosion sounds.
*   `src/Bomber.cpp`: Contains `Audio::play` for bomb placement sounds.
*   `src/Resources.cpp`: Contains `load_sound` which uses `SDL_LoadWAV` and manages a list of `Sound` objects. This is the main resource loading point that needs to be migrated.
*   `reference/clanbomber-2.2.0/src/cbe/SDL/AudioBufferSDL.cpp`: Contains `Mix_LoadWAV` and `Mix_PlayChannel`. This is from the reference implementation and confirms the old pattern.

### List of Legacy Audio Calls

| File | Line | Code | Context | Type | Migration Plan |
|---|---|---|---|---|---|
| `src/Bomb.cpp` | 72 | `Audio::play(explode_sound);` | Bomb explosion | 3D | Replace with `AudioMixer::play_sound_3d("explode", ...)` |
| `src/Bomber.cpp`| 140 | `Audio::play(putbomb_sound);` | Bomber places a bomb | 3D | Replace with `AudioMixer::play_sound_3d("putbomb", ...)` |
| `src/Resources.cpp`| 130 | `Sound* Resources::load_sound(...)` | Sound resource loading | N/A | Replace with `AudioMixer::add_sound(name, AudioMixer::load_sound(path))` |

### Sound File Mapping

The following sound files are loaded in `src/Resources.cpp` and need to be mapped to sound names in the `AudioMixer`.

| Filename | Sound Name |
|---|---|
| `typewriter.wav` | `typewriter` |
| `winlevel.wav` | `winlevel` |
| `klatsch.wav` | `klatsch` |
| `forward.wav` | `forward` |
| `rewind.wav` | `rewind` |
| `stop.wav` | `stop` |
| `wow.wav` | `wow` |
| `joint.wav` | `joint` |
| `horny.wav` | `horny` |
| `schnief.wav` | `schnief` |
| `whoosh.wav` | `whoosh` |
| `break.wav` | `break` |
| `clear.wav` | `clear` |
| `menu_back.wav` | `menu_back` |
| `hurry_up.wav` | `hurry_up` |
| `time_over.wav` | `time_over` |
| `crunch.wav` | `crunch` |
| `die.wav` | `die` |
| `explode.wav` | `explode` |
| `putbomb.wav` | `putbomb` |
| `deepfall.wav` | `deepfall` |
| `corpse_explode.wav` | `corpse_explode` |
| `splash1a.wav` | `splash1` |
| `splash2a.wav` | `splash2` |

## Task 2: Migration Code Generation

This section will be filled with the code replacements for each legacy call.

## Task 3: Audio Resource Loading Migration

The `Resources::init()` function will be modified to load all sounds into the `AudioMixer`. The `Resources::load_sound` function and the `sounds` map will be removed.
