# ClanBomber GPU Rendering Corruption Investigation

## Problem Description

**Issue**: ClanBomber GPU rendering shows visual corruption instead of proper sprites
- **Without AddressSanitizer**: Random RGB noise that changes between executions
- **With AddressSanitizer**: Font text letters appear instead of sprites
- **SDL Rendering**: Works perfectly (no corruption)
- **Pattern**: Each sprite generates specific corruption - not random

## Investigation Timeline & Tests

### Phase 1: Initial GPU System Implementation
- ✅ **Successfully implemented**: Dynamic UV coordinate calculation system
- ✅ **Successfully implemented**: Texture metadata registry (960x40 atlas detection)  
- ✅ **Successfully implemented**: SimpleVertex struct with position/texCoord
- ✅ **Working**: Vertex buffer batching system
- ✅ **Working**: Shader compilation and program linking

### Phase 2: Texture Loading & Format Issues
#### Tests Performed:
- ❌ **FAILED**: Converting PNG textures to RGB format with ImageMagick
- ❌ **FAILED**: Converting PNG textures to RGBA8888 format  
- ❌ **FAILED**: Forcing texture format consistency in SDL_ConvertSurface
- ❌ **FAILED**: Using GL_NEAREST filtering instead of GL_LINEAR
- ❌ **FAILED**: Multiple texture format conversions with ImageMagick

#### Conclusion:
**Texture format/loading is NOT the issue** - corruption persists with any format

### Phase 3: UV Coordinate System Testing  
#### Tests Performed:
- ✅ **PASSED**: UV Debug shader shows clean, correct coordinates 
- ✅ **PASSED**: Dynamic atlas calculation (960x40 → 24 sprites of 40x40)
- ✅ **PASSED**: Texture metadata registration system
- ❌ **FAILED**: Solid red texture (960x40) - still shows corruption
- ❌ **FAILED**: Simple blue/green test sprites - still shows corruption

#### Conclusion:
**UV coordinates are perfect** - corruption exists even with solid color textures

### Phase 4: Memory Corruption Investigation
#### Critical Discovery with AddressSanitizer:
- **Without ASAN**: Random RGB noise 
- **With ASAN**: Font text letters visible
- **Conclusion**: Memory layout affects what corrupted data is displayed

#### Tests Performed:
- ✅ **SUCCESS**: AddressSanitizer eliminated random memory corruption
- ❌ **FAILED**: Zero-initialization `SimpleVertex vertex = {}`
- ❌ **FAILED**: Explicit `memset(&vertex, 0, sizeof(SimpleVertex))`  
- ❌ **FAILED**: `__attribute__((packed))` struct definition
- ❌ **FAILED**: Fixed UV coordinates (0.0-1.0) instead of calculated

#### SDL_TTF Interference Investigation:
- ✅ **CONFIRMED**: SDL_TTF creates font textures that interfere with OpenGL
- ❌ **FAILED**: Texture unit separation (using GL_TEXTURE1 instead of GL_TEXTURE0)
- ❌ **FAILED**: OpenGL state cleanup before sprite rendering
- ❌ **FAILED**: Forced texture regeneration (caused crashes in NVIDIA drivers)

### Phase 5: GPU Pipeline & State Testing
#### Tests Performed:
- ❌ **FAILED**: Disabling GL_DEPTH_TEST for 2D rendering
- ❌ **FAILED**: Disabling GL_MULTISAMPLE (MSAA)
- ❌ **FAILED**: OpenGL texture state cleanup with glBindTexture(GL_TEXTURE_2D, 0)

#### Conclusion:
**OpenGL state management is correct** - problem is deeper

## Current Status

### What Works ✅
- SDL rendering (perfect sprites)
- UV coordinate calculation and debug visualization
- Texture loading and metadata registration  
- Vertex buffer creation and upload
- Shader compilation and program linking
- OpenGL context creation and management

### What's Broken ❌
- Fragment shader texture sampling produces corruption
- Each sprite generates specific corruption pattern
- Memory layout affects corruption type (ASAN changes it)

## Key Observations

1. **Corruption is sprite-specific**: Each sprite produces consistent corruption
2. **Structure is maintained**: Sprite boundaries and positions are correct
3. **SDL vs OpenGL**: Only OpenGL pipeline is affected
4. **Memory-sensitive**: AddressSanitizer changes the corruption pattern
5. **Not random**: Same sprite → same corruption (without ASAN)

## Next Steps

1. **Test minimal fragment shader** (solid colors only, no texture sampling)
2. **Compare with working gpu-test project** in `testing/gpu-test`
3. **Incrementally add complexity** to find exact breaking point
4. **Focus on fragment shader** and texture sampling operations

## Files Modified During Investigation

- `src/GPUAcceleratedRenderer.h` - SimpleVertex struct, texture metadata
- `src/GPUAcceleratedRenderer.cpp` - UV calculation, vertex buffer handling
- `src/Resources.cpp` - Texture format conversion attempts
- `src/Game.cpp` - OpenGL state management
- `data/pics/` - Multiple texture format tests

## Testing Environment

- **OS**: Linux 6.14.0-27-generic
- **GPU**: NVIDIA GeForce RTX 3050 (Driver 570.169)
- **OpenGL**: 4.6.0 NVIDIA
- **SDL**: SDL3
- **Compiler**: GCC 14.2.0 with AddressSanitizer