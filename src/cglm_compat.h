#ifndef CGLM_COMPAT_H
#define CGLM_COMPAT_H

#include <cglm/cglm.h>

// Windows compatibility macros for cglm initialization
#ifdef _WIN32
    // Replace (mat4){} with proper C++ initialization
    #define MAT4_ZERO {{0.0f}}
    #define MAT4_IDENTITY GLM_MAT4_IDENTITY_INIT
    #define VEC3_ZERO {0.0f, 0.0f, 0.0f}
    #define VEC4_ZERO {0.0f, 0.0f, 0.0f, 0.0f}
    
    // Helper macros for common initializations
    #define vec3_init(x, y, z) {x, y, z}
    #define vec4_init(x, y, z, w) {x, y, z, w}
    #define mat4_identity() GLM_MAT4_IDENTITY_INIT
#else
    // Linux/GCC supports compound literals
    #define MAT4_ZERO (mat4){{0.0f}}
    #define MAT4_IDENTITY GLM_MAT4_IDENTITY_INIT
    #define VEC3_ZERO (vec3){0.0f, 0.0f, 0.0f}
    #define VEC4_ZERO (vec4){0.0f, 0.0f, 0.0f, 0.0f}
    
    #define vec3_init(x, y, z) (vec3){x, y, z}
    #define vec4_init(x, y, z, w) (vec4){x, y, z, w}
    #define mat4_identity() GLM_MAT4_IDENTITY_INIT
#endif

#endif // CGLM_COMPAT_H