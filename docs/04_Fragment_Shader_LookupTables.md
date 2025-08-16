# Fragment Shader con Lookup Tables: La Revolución del Ruido

El fragment shader es donde ocurre la "magia visual" - cada pixel que ves en pantalla pasa por aquí. Una pequeña optimización aquí se multiplica por **millones de pixels**.

## El Problema del Ruido Procedural

### ¿Qué es el Ruido Procedural?
Es una función matemática que genera patrones "aleatorios" pero consistentes. Se usa para:
- **Fuego**: Parpadeos y distorsión de llamas
- **Humo**: Movimiento orgánico de partículas  
- **Agua**: Ondas y reflejos
- **Texturas**: Madera, nubes, ruido de TV

### ❌ Implementación Original (Muy Costosa):

```glsl
// Esta función se ejecuta POR CADA PIXEL con efectos
float noise(vec2 p) {
    vec2 i = floor(p);                    // ← OPERACIÓN COSTOSA: División por GPU
    vec2 f = fract(p);                    // ← OPERACIÓN COSTOSA: División por GPU
    f = f * f * (3.0 - 2.0 * f);         // ← Interpolación cúbica: 5 ops por componente
    
    // 4 lookups de random - cada uno hace hash costoso
    float a = random(i);                          // ← 7-8 operaciones matemáticas
    float b = random(i + vec2(1.0, 0.0));        // ← 7-8 operaciones matemáticas  
    float c = random(i + vec2(0.0, 1.0));        // ← 7-8 operaciones matemáticas
    float d = random(i + vec2(1.0, 1.0));        // ← 7-8 operaciones matemáticas
    
    // 3 interpolaciones lineales
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);  // ← 6 operaciones más
}

float random(vec2 co) {
    // Hash function - MUY COSTOSA
    return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);
    //     ↑     ↑   ↑                                    ↑
    //   fract  sin dot                              multiplicación
    //          ↑
    //    SÚPER COSTOSA en GPU
}
```

### 🧮 Cálculo del Costo por Pixel:

Para **un solo pixel** con efecto de fuego:
```
noise() función principal:
  - floor(): 2 ops (x, y)
  - fract(): 2 ops
  - interpolación cúbica: 10 ops
  - 3 mix(): 6 ops
  Total: 20 ops

4 × random():
  - dot(): 2 ops
  - sin(): 8-12 ops (MUY COSTOSA)  
  - multiplicación: 1 op
  - fract(): 1 op
  Total por random(): ~12-15 ops
  4 × random(): 48-60 ops

TOTAL POR PIXEL: 68-80 operaciones matemáticas pesadas
```

### 📊 Impacto Real en el Juego:

**Explosión típica (40×40 pixels):**
- **Pixels con efectos**: 1,600 pixels
- **Operaciones por pixel**: 70 operaciones promedio
- **Total por explosión**: 1,600 × 70 = **112,000 operaciones**
- **A 60 FPS**: 112,000 × 60 = **6,720,000 operaciones por segundo**

**Con múltiples explosiones:**
- 5 explosiones simultáneas = **33,600,000 operaciones/segundo**
- ¡Solo para el ruido de los efectos de fuego!

## ✅ Solución: Lookup Table (LUT)

### Concepto Base
En lugar de calcular ruido en tiempo real, lo **precalculamos** y guardamos en una textura.

```cpp
// EN EL CPU - Solo una vez al iniciar el juego
void create_noise_lut() {
    const int size = 256;  // 256×256 = 65,536 valores precalculados
    float noise_data[size * size];
    
    // Calculamos todo el ruido UNA VEZ
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            float fx = (float)x / size;
            float fy = (float)y / size;
            
            // Usamos la función costosa SOLO AQUÍ
            noise_data[y * size + x] = expensive_noise_calculation(fx, fy);
        }
    }
    
    // Subimos a GPU como textura
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, size, size, 0, 
                 GL_RED, GL_FLOAT, noise_data);
}
```

### Fragment Shader Optimizado:

```glsl
// Textura precalculada
uniform sampler2D uNoiseLUT;

// Función súper rápida
float fastNoise(vec2 p) {
    // UNA SOLA operación de texture lookup
    return texture(uNoiseLUT, p * 0.00390625).r;  // 0.00390625 = 1/256
    //     ↑
    //   Hardware especializado - SÚPER RÁPIDO
}
```

### 🧮 Nuevo Costo por Pixel:

```
fastNoise():
  - texture(): 1 operación (hardware dedicado)
  - multiplicación: 1 operación  
  Total: 2 operaciones

TOTAL POR PIXEL: 2 operaciones vs 70 anteriores
MEJORA: 35x más rápido por pixel
```

### 📊 Impacto Real:

**Explosión típica (40×40 pixels):**
- **Antes**: 112,000 operaciones
- **Después**: 1,600 × 2 = **3,200 operaciones**
- **Mejora**: 35x más rápido

**Con múltiples explosiones:**
- **Antes**: 33,600,000 operaciones/segundo
- **Después**: 960,000 operaciones/segundo  
- **Mejora**: 35x menos carga en GPU

## Efectos Visuales Optimizados

Ahora analicemos cómo optimizamos cada efecto específico:

### 1. Efecto de Partículas (Glow/Brillo)

#### ❌ Versión Original:
```glsl
if (uEffectType == 1) {
    // Brillo pulsante - cálculo costoso por pixel
    float glow = 1.0 + 0.5 * sin(uTime * 12.0 + WorldPos.x * 0.1);
    finalColor.rgb *= glow;
    
    // Distancia radial - raíz cuadrada costosa
    vec2 center = TexCoord - vec2(0.5);
    float radialGlow = 1.0 - length(center) * 2.0;  // ← length() = sqrt()
    radialGlow = max(0.0, radialGlow);
    finalColor.rgb += vec3(radialGlow * 0.3, radialGlow * 0.2, 0.0);
    
    // Transparencia pulsante
    finalColor.a *= 0.8 + 0.2 * sin(uTime * 8.0);  // ← Otro sin() costoso
}
```

#### ✅ Versión Optimizada:
```glsl
if (EffectMode == 1) {
    // Brillo con valores precomputados
    float glow = 1.0 + 0.5 * sin(uTimeData.w * 6.0 + WorldPos.x * 0.1);
    finalColor.rgb *= glow;
    
    // Distancia radial optimizada - evitamos sqrt()
    vec2 center = TexCoord - vec2(0.5);
    float radialGlow = max(0.0, 1.0 - dot(center, center) * 4.0);  // ← dot() muy rápido
    finalColor.rgb += vec3(radialGlow * 0.3, radialGlow * 0.2, 0.0);
    
    // Transparencia con sin precomputado
    finalColor.a *= 0.8 + 0.2 * uTimeData.y;  // ← Usa sin precalculado
}
```

**Optimizaciones:**
1. **sin() precomputado**: `uTimeData.y` vs calcular `sin(uTime * 8.0)`
2. **dot() vs length()**: `dot(v,v)` es mucho más rápido que `sqrt(dot(v,v))`
3. **EffectMode plano**: Mejor branching efficiency

### 2. Efecto de Explosión (Fuego/Calor)

#### ❌ Versión Original:
```glsl
if (uEffectType == 2) {
    // Distorsión de calor - múltiples noise calls
    vec2 distortionOffset = vec2(
        smoothNoise(TexCoord * 10.0 + uTime * 2.0) - 0.5,      // ← 70 ops
        smoothNoise(TexCoord * 8.0 + uTime * 1.5) - 0.5       // ← 70 ops  
    ) * 0.03;
    
    vec2 distortedUV = TexCoord + distortionOffset;
    texColor = texture(uTexture, distortedUV);
    finalColor = texColor * Color;
    
    // Más efectos costosos...
    float flame = smoothNoise(TexCoord * 15.0 + vec2(0, uTime * 3.0));  // ← 70 ops más
    if (flame > 0.7) {
        finalColor.rgb += vec3(0.8, 0.3, 0.0) * (flame - 0.7) * 3.0;
    }
}
```

**Costo por pixel:** ~210 operaciones

#### ✅ Versión Optimizada:
```glsl
if (EffectMode == 2) {
    // Distorsión con LUT - súper rápida
    float noiseVal = fastNoise(TexCoord * 10.0 + uTimeData.xy);        // ← 2 ops
    vec2 distortionOffset = vec2(noiseVal - 0.5, 
                                fastNoise(TexCoord * 8.0 + uTimeData.yx) - 0.5) * 0.03;  // ← 2 ops más
    
    texColor = texture(uTexture, TexCoord + distortionOffset);
    finalColor = texColor * Color;
    
    // Efecto de calor optimizado
    float heat = sin(uTimeData.w * 3.0 + WorldPos.y * 0.02);
    finalColor.r += heat * 0.3;
    finalColor.g += heat * 0.1;
    
    // Llamas con single noise check + branching optimizado
    if (noiseVal > 0.7) {  // ← Reusamos noiseVal calculado arriba
        finalColor.rgb += FIRE_COLOR_END * (noiseVal - 0.7) * 3.0;
    }
}
```

**Costo por pixel:** ~8-10 operaciones

**Mejora:** 21x más rápido (210 ops → 10 ops)

### 3. Efecto de Invencibilidad (Chromatic Aberration)

#### ❌ Versión Original:
```glsl
if (uEffectType == 3) {
    // Chromatic aberration - múltiples texture lookups
    float aberrationStrength = 0.008 * sin(uTime * 10.0);  // ← sin() costoso
    
    float r = texture(uTexture, TexCoord + vec2(aberrationStrength, 0)).r;
    float g = texture(uTexture, TexCoord).g;
    float b = texture(uTexture, TexCoord - vec2(aberrationStrength, 0)).b;
    
    finalColor = vec4(r, g, b, texColor.a) * Color;
    
    // Rainbow shimmer - múltiples sin() calls
    float shimmer = sin(uTime * 8.0 + WorldPos.x * 0.05 + WorldPos.y * 0.03);
    vec3 rainbow = vec3(
        sin(shimmer + 0.0) * 0.5 + 0.5,      // ← 3 sin() costosos
        sin(shimmer + 2.094) * 0.5 + 0.5,
        sin(shimmer + 4.188) * 0.5 + 0.5
    );
    finalColor.rgb = mix(finalColor.rgb, rainbow, 0.3);
}
```

#### ✅ Versión Optimizada:
```glsl
if (EffectMode == 3) {
    // Chromatic aberration con sin precomputado  
    float aberration = 0.008 * sin(uTimeData.w * 5.0);  // ← Usa tiempo precomputado
    
    float r = texture(uTexture, TexCoord + vec2(aberration, 0)).r;
    float b = texture(uTexture, TexCoord - vec2(aberration, 0)).b;
    
    finalColor = vec4(r, texColor.g, b, texColor.a) * Color;
    
    // Rainbow con valores precomputados
    float shimmer = uTimeData.y + WorldPos.x * 0.05 + WorldPos.y * 0.03;  // ← Base sin precomputado
    vec3 rainbow = vec3(
        sin(shimmer + RAINBOW_PHASE.x) * 0.5 + 0.5,  // ← Constantes precomputadas
        sin(shimmer + RAINBOW_PHASE.y) * 0.5 + 0.5,
        sin(shimmer + RAINBOW_PHASE.z) * 0.5 + 0.5
    );
    finalColor.rgb = mix(finalColor.rgb, rainbow, 0.3);
    
    finalColor.a *= 0.6 + 0.4 * sin(uTimeData.w * 6.0);
}
```

**Optimizaciones:**
1. **Sin base precomputado**: `uTimeData.y` reduce 1 sin() call
2. **Constantes RGB phases**: `RAINBOW_PHASE` vector precalculado  
3. **Menos texture lookups**: Solo R y B, reusa G del lookup base

## Técnicas de Optimización Avanzadas

### 1. Flat Attributes para Mejor Branching

```glsl
// ❌ MALO: uEffectType uniform causa divergencia
uniform int uEffectType;  // Todos los pixels en el frame tienen mismo valor

void main() {
    if (uEffectType == 1) { /* código A */ }      // Si hay mezcla de efectos,
    if (uEffectType == 2) { /* código B */ }      // GPU ejecuta TODAS las ramas
    if (uEffectType == 3) { /* código C */ }      // para cada warp
}

// ✅ BUENO: flat EffectMode por sprite
flat in int EffectMode;   // Cada sprite puede tener efecto diferente

void main() {
    if (EffectMode == 1) { /* código A */ }       // Todos los pixels del sprite
    if (EffectMode == 2) { /* código B */ }       // ejecutan la misma rama
    if (EffectMode == 3) { /* código C */ }       // = mejor efficiency
}
```

### 2. Constantes vs Uniforms

```glsl
// ❌ Menos eficiente
uniform float fireColorStart[3];  // Requiere memory lookup
uniform float fireColorEnd[3];

// ✅ Más eficiente  
const vec3 FIRE_COLOR_START = vec3(1.0, 1.0, 0.8);   // Compilado en el shader
const vec3 FIRE_COLOR_END = vec3(0.8, 0.3, 0.0);     // Puede usar registros especiales
```

### 3. Reuso de Cálculos

```glsl
// ❌ MALO: Calculamos noise múltiples veces
if (effect == BLOOD) {
    float noise1 = fastNoise(uv * 20.0);  
    float noise2 = fastNoise(uv * 18.0);
    // ... usar noise1 y noise2
}

// ✅ BUENO: Calculamos una vez, reusamos
if (effect == BLOOD) {
    float bloodNoise = fastNoise(uv * 20.0);
    vec2 bloodUV = uv + vec2(bloodNoise - 0.5, fastNoise(uv * 18.0) - 0.5);
    
    // Reusamos bloodNoise más tarde
    float bloodMask = step(0.85, bloodNoise);  // ← Reuso!
}
```

## Métricas de Performance

### Comparación Final por Efecto:

| Efecto | Ops Originales | Ops Optimizadas | Mejora |
|--------|---------------|------------------|---------|
| Partículas | 25-30 | 8-10 | 3x |
| Explosión | 200-250 | 8-12 | 20x |
| Invencibilidad | 40-50 | 15-20 | 2.5x |
| Sangre | 150-180 | 10-15 | 15x |

### Performance Global:

**Escenario típico (5 explosiones + 10 bombers + efectos):**
- **Pixels afectados**: ~50,000 pixels
- **Antes**: 50,000 × 150 ops promedio = 7,500,000 ops/frame
- **Después**: 50,000 × 12 ops promedio = 600,000 ops/frame
- **Mejora**: 12.5x menos carga en GPU

**A 60 FPS:**
- **Antes**: 450,000,000 operaciones/segundo
- **Después**: 36,000,000 operaciones/segundo
- **GPU liberada**: 92% menos trabajo para efectos

Esta optimización masiva permite usar esos cycles ahorrados para:
- Más partículas (50,000 vs 10,000)
- Efectos más complejos
- Mejor framerate
- Soporte para hardware más viejo

En el próximo documento analizaremos el compute shader optimizado para el sistema de partículas masivo.