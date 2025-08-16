# Audio Testing Checklist

This checklist is to ensure that the audio migration was successful and that all sounds are playing correctly.

## 1. Sound Effects

### 1.1. 3D Positional Sounds

| Sound Name | Context | How to Test | Expected Result |
|---|---|---|---|
| `explode` | Bomb explosion | Explode a bomb | Sound should come from the bomb's location. |
| `putbomb` | Placing a bomb | Place a bomb | Sound should come from the bomber's location. |
| `die` | Bomber dies | Kill a bomber | Sound should come from the bomber's location. |
| `corpse_explode` | Corpse explodes | Explode a corpse | Sound should come from the corpse's location. |
| `break` | Box destruction | Destroy a box | Sound should come from the box's location. |
| `schnief` | Negative extra pickup | Pick up a negative extra | Sound should come from the extra's location. |
| `wow` | Positive extra pickup | Pick up a positive extra | Sound should come from the extra's location. |
| `winlevel` | Winning a level | Win a level | Sound should come from the winning player's location. |
| `time_over` | Time is over | Let the time run out | Sound should come from the center of the screen. |

### 1.2. 2D UI Sounds

| Sound Name | Context | How to Test | Expected Result |
|---|---|---|---|
| `typewriter` | Typing in a menu | Type in a menu | Sound should be clear and not positional. |
| `klatsch` | Menu navigation | Navigate a menu | Sound should be clear and not positional. |
| `forward` | Menu navigation | Navigate a menu | Sound should be clear and not positional. |
| `rewind` | Menu navigation | Navigate a menu | Sound should be clear and not positional. |
| `stop` | Menu navigation | Navigate a menu | Sound should be clear and not positional. |
| `menu_back` | Going back in a menu | Go back in a menu | Sound should be clear and not positional. |

## 2. Music

| Music Name | Context | How to Test | Expected Result |
|---|---|---|---|
| `music` | Background music | Start a game | Music should play correctly. |

## 3. General

*   [ ] All sounds should be audible.
*   [ ] There should be no crashes related to the audio system.
*   [ ] The volume of the sounds should be balanced.
*   [ ] The 3D sounds should correctly reflect the position of the sound source.
