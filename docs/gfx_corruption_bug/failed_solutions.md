# Failed Solutions Documentation

This document details all attempted solutions that did NOT fix the corruption issue.

## Texture Format Solutions (All Failed)

### ImageMagick Conversions Attempted
```bash
# Convert to RGB format
convert data/pics/maptiles.png -background black -alpha remove -type TrueColor data/pics/maptiles_fixed.png

# Force RGBA format  
convert data/pics/maptiles.png -alpha set -background transparent -type TrueColorAlpha data/pics/maptiles_rgba.png

# Force 8-bit alpha
convert data/pics/maptiles.png -alpha set -channel A -evaluate set 100% +channel data/pics/maptiles_full_alpha.png

# Solid red test texture
convert -size 960x40 xc:red data/pics/maptiles.png

# Simple sprite atlas test
convert -size 40x40 xc:blue \( +clone -fill green -draw "rectangle 10,10 30,30" \) +append \( +clone +clone +clone +clone +clone +clone +clone +clone +clone +clone +clone \) +append data/pics/test_simple_sprites.png
```

**Result**: Corruption persists with ALL texture formats and types

### SDL Surface Format Conversions
```cpp
// Attempted in Resources.cpp
SDL_Surface* rgba_surface = SDL_ConvertSurface(surface, SDL_PIXELFORMAT_RGBA8888);
```

**Result**: Same corruption patterns

### OpenGL Texture Filtering
```cpp
// Tested both linear and nearest
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  
glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
```

**Result**: No change in corruption

## Memory Initialization Solutions (All Failed)

### Zero Initialization Attempts
```cpp
// Attempt 1: Brace initialization
SimpleVertex vertex = {};

// Attempt 2: Explicit memset
SimpleVertex vertex;
memset(&vertex, 0, sizeof(SimpleVertex));
```

**Result**: Corruption persists

### Struct Packing
```cpp
struct __attribute__((packed)) SimpleVertex {
    float position[2]; 
    float texCoord[2];
};
```

**Result**: Corruption became more "packed" but still present

### Fixed UV Coordinates 
```cpp
// Instead of calculated UVs, use fixed values
float sprite_u_start = 0.0f;
float sprite_u_end = 1.0f;
float sprite_v_start = 0.0f; 
float sprite_v_end = 1.0f;
```

**Result**: Same corruption patterns

## OpenGL State Solutions (All Failed)

### Depth Testing
```cpp
glDisable(GL_DEPTH_TEST);  // Instead of glEnable(GL_DEPTH_TEST)
```

**Result**: No change

### Multisampling  
```cpp
glDisable(GL_MULTISAMPLE);  // Instead of glEnable(GL_MULTISAMPLE)
```

**Result**: No change

### Texture Unit Separation
```cpp
// Use texture unit 1 to avoid SDL_TTF conflicts
glUniform1i(u_texture, 1);
glActiveTexture(GL_TEXTURE1);
```

**Result**: Black screen (textures not bound correctly)

### Texture State Cleanup
```cpp
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, 0);  // Unbind first
glBindTexture(GL_TEXTURE_2D, current_texture);
```

**Result**: Same corruption

### Forced Texture Regeneration
```cpp
// Dangerous approach that caused crashes
if (name == "maptiles" && tex_info->gl_texture != 0) {
    glDeleteTextures(1, &tex_info->gl_texture);  // CRASH!
    tex_info->gl_texture = 0;
}
```

**Result**: SEGV in NVIDIA driver

## SDL_TTF Interference Solutions (Partially Successful)

### Observation
- **Without AddressSanitizer**: Random RGB noise
- **With AddressSanitizer**: Font text letters ("ENT", "G", etc.)

### Failed Attempts
- Texture unit separation 
- OpenGL state isolation
- Texture regeneration (crashed)

### What This Revealed
The memory corruption is real, but AddressSanitizer changes the memory layout so we see different corrupted data (font textures instead of random bytes).

## Vertex Buffer Solutions (All Failed)

### Buffer Size Validation
Added extensive debugging for buffer sizes, vertex counts, and data validation - all showed correct values.

### Buffer Recreation
```cpp
if (buffer_size == 0) {
    size_t expected_buffer_size = MAX_QUADS * 4 * sizeof(SimpleVertex);
    glBufferData(GL_ARRAY_BUFFER, expected_buffer_size, nullptr, GL_DYNAMIC_DRAW);
}
```

**Result**: Buffer operations work correctly, corruption persists

## What These Failures Tell Us

1. **Not a texture format issue** - every format produces corruption
2. **Not a memory initialization issue** - explicit zeroing doesn't help
3. **Not an OpenGL state issue** - state changes don't affect it
4. **Not a vertex buffer issue** - data arrives correctly at GPU
5. **Not a UV coordinate issue** - debug shader shows correct UVs

## Conclusion

The corruption happens **between vertex shader output and fragment shader texture sampling**. The issue is likely:

- Fragment shader texture sampling logic
- GPU driver interaction with specific OpenGL calls
- Subtle pipeline state that we haven't identified
- Hardware-specific issue with our OpenGL usage pattern