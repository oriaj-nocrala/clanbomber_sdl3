# Conceptos Básicos de Shaders

## ¿Qué son los Shaders?

Los **shaders** son pequeños programas que se ejecutan directamente en la GPU (tarjeta gráfica) para procesar gráficos. Imagina que son como "filtros" o "efectos" que se aplican a cada pixel o vértice de tu juego.

### Analogía Sencilla
Piensa en los shaders como una fábrica:
- **Vertex Shader**: Es como el departamento que decide dónde van las cosas (posición de vértices)
- **Fragment Shader**: Es como el departamento de pintura que decide qué color tiene cada pixel
- **Compute Shader**: Es como un departamento general que puede hacer cálculos complejos (como física de partículas)

## Tipos de Shaders en Nuestro Juego

### 1. Vertex Shader
```glsl
// Este shader se ejecuta una vez por cada vértice
void main() {
    // Transforma la posición 2D del sprite
    gl_Position = projection * view * model * vec4(position, 0.0, 1.0);
}
```

**¿Qué hace?**
- Recibe la posición original de cada esquina del sprite (vértice)
- La transforma según la cámara, rotación, escala, etc.
- Envía la posición final donde debe aparecer en pantalla

**En nuestro juego:**
- Mueve los bombers por el mapa
- Rota las explosiones
- Hace que las partículas floten con efectos de onda

### 2. Fragment Shader
```glsl
// Este shader se ejecuta una vez por cada pixel
void main() {
    vec4 color = texture(sprite_texture, uv_coordinates);
    FragColor = color;
}
```

**¿Qué hace?**
- Se ejecuta para CADA pixel que va a dibujarse
- Decide el color final de cada pixel
- Puede aplicar efectos como brillo, distorsión, transparencia

**En nuestro juego:**
- Colorea los sprites de los bombers
- Crea efectos de fuego en las explosiones
- Hace efectos de sangre y gore
- Genera efectos de invencibilidad con colores rainbow

### 3. Compute Shader
```glsl
// Este shader puede hacer cualquier cálculo
layout(local_size_x = 64) in;
void main() {
    uint particle_id = gl_GlobalInvocationID.x;
    // Actualiza física de partícula
}
```

**¿Qué hace?**
- Ejecuta cálculos generales en paralelo
- Perfecto para física, simulaciones, AI
- Puede procesar miles de elementos a la vez

**En nuestro juego:**
- Simula la física de 50,000+ partículas de sangre y fuego
- Aplica gravity, colisiones, viento
- Actualiza colores y vida de partículas

## Pipeline de Renderizado

```
1. Aplicación (CPU) → 2. Vertex Shader → 3. Rasterización → 4. Fragment Shader → 5. Pantalla
```

### Paso a Paso en Nuestro Juego:

1. **CPU**: "Quiero dibujar un bomber en (100, 200)"
2. **Vertex Shader**: "OK, las 4 esquinas del sprite van en estas posiciones de pantalla"
3. **GPU (Rasterización)**: "Necesito colorear 1600 pixels (40x40 sprite)"
4. **Fragment Shader**: "Pixel (105, 203) = color rojo del sprite + efecto de brillo"
5. **Pantalla**: Se ve el bomber coloreado con efectos

## Conceptos Clave para Entender las Optimizaciones

### 1. Paralelismo Masivo
- Un fragment shader se ejecuta para **millones de pixels** simultáneamente
- Una optimización pequeña se multiplica por millones de veces
- Por eso 0.001ms de mejora → 1000ms de mejora total

### 2. Memoria de GPU
- **Registers**: Súper rápida, muy limitada
- **Shared Memory**: Rápida, compartida entre threads cercanos
- **Global Memory**: Lenta, pero mucha capacidad
- **Texture Memory**: Optimizada para lookups 2D

### 3. Branching (if/else)
```glsl
// ❌ MALO: Diferentes threads pueden tomar diferentes caminos
if (particle.type == FIRE) {
    // Código para fuego
} else if (particle.type == SMOKE) {
    // Código para humo
}

// ✅ BUENO: Todos los threads hacen lo mismo
float fire_factor = (particle.type == FIRE) ? 1.0 : 0.0;
color = mix(base_color, fire_color, fire_factor);
```

### 4. Coherencia de Memoria
```glsl
// ❌ MALO: Accesos aleatorios a memoria
float noise = random(position * 12345.67);

// ✅ BUENO: Lookup table precomputada
float noise = texture(noise_lut, position * scale).r;
```

## ¿Por Qué OpenGL 4.6?

### OpenGL 3.3 (Antiguo):
- Shaders básicos
- Menos funciones avanzadas
- Compatible con hardware más viejo

### OpenGL 4.6 (Moderno):
- **Compute Shaders**: Para física y simulaciones
- **Shader Storage Buffers**: Memoria compartida eficiente
- **Mejor optimización del compilador**: GPU puede optimizar mejor
- **Más instrucciones por clock**: Más operaciones por ciclo

## Próximos Documentos

En los siguientes archivos veremos:
- **02**: Análisis detallado de nuestras optimizaciones específicas
- **03**: Vertex Shader optimizado paso a paso
- **04**: Fragment Shader con lookup tables
- **05**: Compute Shader para 50K partículas
- **06**: Medición de performance y resultados

---

*Este es el primer paso para entender cómo optimizamos los shaders de ClanBomber. En el próximo documento analizaremos las optimizaciones específicas que implementamos.*