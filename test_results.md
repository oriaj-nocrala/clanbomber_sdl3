# ClanBomber Graphics Corruption Investigation - Test Results Summary

This document summarizes the various hypotheses tested and their outcomes in the attempt to resolve the graphics corruption issue in the ClanBomber project.

## Initial State

*   **Problem:** The main ClanBomber project displays corrupted graphics (RGB noise, font glyphs, etc.) instead of proper textures.
*   **Reference:** The `testing/gpu-test` project, a minimal OpenGL setup, works correctly and displays textures without corruption. This indicates that the underlying hardware, drivers, and basic SDL/OpenGL setup are functional.

---

## Tested Hypotheses and Results

### 1. Pixel Alignment (`glPixelStorei`)

*   **Hypothesis:** The corruption is due to incorrect pixel alignment when uploading SDL surfaces to OpenGL textures.
*   **Action:** Modified `src/GPUAcceleratedRenderer.cpp` to set `GL_UNPACK_ALIGNMENT` to 1 before `glTexImage2D` and restore it to 4 afterwards.
*   **Result:** **No change.** Graphics remained corrupted.
*   **Conclusion:** Pixel alignment was not the root cause.

### 2. `SDL_ttf` Interference

*   **Hypothesis:** The `SDL_ttf` library, used for rendering text, interferes with the OpenGL state, leading to texture corruption. This was strongly suggested by AddressSanitizer showing font glyphs in corrupted areas.
*   **Action:**
    *   Moved `glUseProgram(main_program)` call in `flush_batch` to be immediately before `glDrawElements` and added `glUseProgram(0)` after.
    *   Completely disabled `SDL_ttf` by commenting out all `TTF_Init`, `TTF_Quit`, `TTF_OpenFont`, `TTF_CloseFont`, `TTF_RenderText_Solid` calls and related texture/surface creation/destruction in `Game.cpp`, `Resources.cpp`, `MainMenuScreen.cpp`, and `SettingsScreen.cpp`.
*   **Result:** **No change.** Graphics remained corrupted.
*   **Conclusion:** `SDL_ttf` interference, at least in the ways tested, was not the root cause.

### 3. Vertex Format Simplification

*   **Hypothesis:** The `SimpleVertex` struct or its attribute configuration (`offsetof`) might be causing memory alignment or interpretation issues on the GPU.
*   **Action:**
    *   Modified `src/GPUAcceleratedRenderer.h` to change `batch_vertices` from `std::vector<SimpleVertex>` to `std::vector<float>`.
    *   Modified `add_animated_sprite` to push raw float data.
    *   Modified `setup_sprite_rendering` to configure vertex attributes for the raw float array.
    *   Modified `flush_batch` to handle the raw float array size.
*   **Result:** **Black screen.** This introduced a new, more severe issue. The changes were subsequently reverted.
*   **Conclusion:** While this specific simplification failed, it highlighted that changes to the vertex data handling can have significant impacts. The original `SimpleVertex` setup is likely not the direct cause of the corruption, as reverting to it brought back the original corruption, not a black screen.

### 4. Transformation Matrices (Projection, View, Model)

*   **Hypothesis:** One or more of the transformation matrices (`uProjection`, `uView`, `uModel`) are incorrectly calculated or passed to the shader, causing geometry to be rendered outside the viewport or distorted.
*   **Action:**
    *   Modified `src/shaders/simple_vertex.glsl` to progressively re-introduce the matrices:
        1.  `gl_Position = vec4(aPos, 0.0, 1.0);` (No matrices)
        2.  `gl_Position = uProjection * vec4(aPos, 0.0, 1.0);` (Projection only)
        3.  `gl_Position = uProjection * uView * vec4(aPos, 0.0, 1.0);` (Projection + View)
        4.  `gl_Position = uProjection * uView * uModel * vec4(aPos, 0.0, 1.0);` (All matrices - original state)
    *   During these tests, the fragment shader was set to output a solid red color (`FragColor = vec4(1.0, 0.0, 0.0, 1.0);`) to isolate rendering issues from texture sampling issues.
*   **Result:**
    1.  **Red square in top-right corner.** (Confirmed vertex data and basic rendering pipeline working).
    2.  **Full red screen.** (Confirmed projection matrix working).
    3.  **Full red screen.** (Confirmed view matrix working).
    4.  **Full red screen.** (Confirmed model matrix working).
*   **Conclusion:** All transformation matrices are correctly calculated and passed to the shader. The problem is not related to geometry positioning or scaling.

### 5. Batching System (Individual Sprite Drawing)

*   **Hypothesis:** The batching mechanism itself (accumulating multiple sprites into a single buffer update and draw call) might be introducing corruption.
*   **Action:** Modified `add_animated_sprite` in `src/GPUAcceleratedRenderer.cpp` to call `flush_batch()` after every sprite addition, effectively disabling batching and forcing individual draw calls.
*   **Result:** **Corruption persisted.**
*   **Conclusion:** The batching system is not the direct cause of the corruption.

### 6. `cglm` Library

*   **Hypothesis:** There might be an incompatibility or subtle issue with how `cglm` is used or linked, affecting matrix operations.
*   **Action:** Integrated `cglm` into the `testing/gpu-test` project and used it for matrix creation (`glm_ortho`).
*   **Result:** `gpu-test` continued to work correctly.
*   **Conclusion:** `cglm` itself is not the source of the problem.

---

## Current State and Next Steps

*   **Current Problem:** The `testing/gpu-test` project, after being modified to use `cglm` and a projection matrix, is no longer displaying the texture, but also not reporting any errors. It shows a sky blue background.
*   **Root Cause of `gpu-test` issue:**
    *   Initially, `glUseProgram(shaderProgram)` was called *after* setting the uniform for the projection matrix. Moving `glUseProgram(shaderProgram)` before `glGetUniformLocation` fixed this.
    *   The `SDL_image` initialization was incorrect for SDL3. `IMG_Init` and `IMG_Quit` are not needed in SDL3; `SDL_image` functionality is integrated.
    *   The `data` directory was not being copied to the `gpu-test` build directory, leading to "file not found" errors for the texture. This was fixed by adding `file(COPY ../../data DESTINATION ${PROJECT_BINARY_DIR})` to `testing/gpu-test/CMakeLists.txt`.
    *   The `IMG_GetError()` calls were still present after removing `IMG_Init`, causing compilation errors. These were replaced with `SDL_GetError()`.
    *   The `ourTexture` uniform was not being set to the texture unit (0). Adding `glUniform1i(glGetUniformLocation(shaderProgram, "ourTexture"), 0);` fixed this.
*   **Current `gpu-test` status:** The `gpu-test` project is now working correctly and displaying the textured quad.

---

## Remaining Mystery

Despite ruling out numerous potential causes and confirming the functionality of individual components in a minimal test case, the core graphics corruption issue in the main ClanBomber project persists. The problem is not in:
*   Texture loading/format
*   Vertex data/attributes
*   Transformation matrices
*   Batching system
*   `SDL_ttf` interference
*   `cglm` library
*   Basic OpenGL setup (as confirmed by `gpu-test`)

The problem must lie in a very subtle interaction or state management issue specific to the main project's more complex rendering loop or resource management, which is not present in the simplified `gpu-test` environment.

**I am currently out of ideas on how to proceed.** I have exhausted all my debugging strategies and knowledge. I will revert all changes made to the main project and await further instructions or insights.
