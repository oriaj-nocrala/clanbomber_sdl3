# TODO para Gemini (1M Context Window)

## Tarea: Migración Completa de Audio de SDL2 Mixer a AudioMixer Personalizado

### Contexto del Problema

ClanBomber actualmente tiene una **mezcla inconsistente** de sistemas de audio:

1. **Código original (SDL2 Mixer)** - Sistema viejo, menos eficiente
2. **AudioMixer moderno** - Sistema nuevo con audio 3D posicional

El problema es que hay **cientos de llamadas** de audio esparcidas por todo el codebase que necesitan ser migradas manualmente. Esta es una tarea **perfecta para una IA con 1M tokens** porque requiere:

- Leer y analizar **TODO** el codebase completo
- Identificar **todos** los archivos con audio
- Entender el **contexto** de cada llamada
- Aplicar **transformaciones consistentes** pero específicas al contexto

---

## 🎯 **TAREA PRINCIPAL**

Migra **TODAS** las llamadas de audio en el codebase de:
- `Mix_PlayChannel()`, `Mix_LoadWAV()`, etc. (SDL2 Mixer)  
- A nuestro `AudioMixer::play_sound_3d()` moderno

### Archivos a Analizar (lee TODOS):

```bash
# Código fuente principal
../src/*.cpp
../src/*.h

# Código de referencia (para entender la lógica original)
../reference/clanbomber-2.2.0/src/*.cpp
../reference/clanbomber-2.2.0/src/*.h

# Build outputs (para ver qué se está compilando)
*.cpp
*.h
```

---

## 📋 **TRANSFORMACIONES REQUERIDAS**

### 1. Identificación de Patrones

**BUSCA** estos patrones en TODO el código:

```cpp
// ❌ Patrones viejos a reemplazar:
Mix_PlayChannel(-1, sound_effect, 0);
Mix_LoadWAV("path/to/sound.wav");  
Mix_PlaySound(sound_handle);
Mix_Chunk* sound = Mix_LoadWAV_RW(...);
Mix_Volume(channel, volume);
Mix_HaltChannel(channel);

// También busca variantes como:
play_sound("bomb_explode");  // Funciones wrapper custom
Audio::play(sound_name);     // Otros wrappers
```

### 2. Transformaciones Específicas por Contexto

#### **A. Sonidos de Explosión**
```cpp
// ❌ ANTES:
Mix_PlayChannel(-1, explosion_sound, 0);

// ✅ DESPUÉS:  
AudioPosition explosion_pos(bomb_x, bomb_y, 0.0f);
AudioMixer::play_sound_3d("explosion", explosion_pos, 800.0f);
```

#### **B. Sonidos de Bomber (pasos, muerte, etc.)**
```cpp
// ❌ ANTES:
Mix_PlayChannel(-1, step_sound, 0);

// ✅ DESPUÉS:
AudioPosition bomber_pos(bomber->get_x(), bomber->get_y(), 0.0f);  
AudioMixer::play_sound_3d("step", bomber_pos, 300.0f);
```

#### **C. Sonidos de UI (menús, botones)**
```cpp
// ❌ ANTES:  
Mix_PlayChannel(-1, button_sound, 0);

// ✅ DESPUÉS:
// UI sounds no necesitan posición 3D
AudioMixer::play_sound("button_click");
```

#### **D. Sonidos ambientes/música**
```cpp
// ❌ ANTES:
Mix_PlayMusic(background_music, -1);

// ✅ DESPUÉS: 
// Música también sin 3D
AudioMixer::play_sound("background_music");
```

### 3. Mapeo de Archivos de Sonido

**CREA** un mapeo completo de todos los sonidos referenciados:

```cpp
// Analiza TODOS los archivos .wav, .ogg, etc. mencionados y crea:
std::map<std::string, std::string> sound_file_mapping = {
    {"explosion.wav", "explosion"},
    {"bomb_explode.wav", "bomb_explode"}, 
    {"step1.wav", "step"},
    {"button.wav", "button_click"},
    {"death.wav", "death"},
    {"collect.wav", "item_pickup"},
    // ... TODOS los sonidos encontrados
};
```

---

## 🎯 **TAREAS ESPECÍFICAS**

### Task 1: Análisis Completo del Audio Legacy

1. **Lee TODO el codebase** (usa tu 1M context window)
2. **Identifica TODOS** los archivos que usan audio
3. **Lista TODAS** las llamadas de audio encontradas con:
   - Archivo y línea
   - Función que llama audio  
   - Contexto (explosión, UI, bomber, etc.)
   - Tipo de sonido (3D posicional vs normal)

**Output esperado:**
```
AUDIO_CALLS_FOUND.md:
- src/Bomb.cpp:123: Mix_PlayChannel() - Explosión de bomba [3D needed]
- src/MainMenu.cpp:67: Mix_PlayChannel() - Botón de menú [2D UI sound]  
- src/Bomber.cpp:234: play_sound() - Pasos de bomber [3D needed]
- src/GameplayScreen.cpp:445: Mix_PlayChannel() - Sonido de victoria [2D UI sound]
- ... (TODAS las llamadas encontradas)
```

### Task 2: Generación de Código de Migración

Para **CADA** llamada de audio encontrada, genera el código de reemplazo:

**Formato:**
```cpp
// ============= ARCHIVO: src/Bomb.cpp LÍNEA: 123 =============
// ❌ CÓDIGO ORIGINAL:
Mix_PlayChannel(-1, explosion_sound, 0);

// ✅ CÓDIGO REEMPLAZADO:
AudioPosition explosion_pos(get_x(), get_y(), 0.0f);
AudioMixer::play_sound_3d("explosion", explosion_pos, 800.0f);

// 💡 EXPLICACIÓN: 
// - Explosión necesita audio 3D posicional
// - Radio de 800px para efectos de distancia
// - Posición basada en coordenadas de la bomba
```

### Task 3: Detección de Audio Resources Loading

**Encuentra TODOS** los lugares donde se cargan recursos de audio:

```cpp
// Busca patrones como:
Mix_LoadWAV("data/sounds/explosion.wav");
Mix_LoadMUS("data/music/background.ogg");  
Audio::load_sound("bomb", "sounds/bomb.wav");
```

**Genera código para migrarlos a:**
```cpp
// En el init del juego:
void init_audio_resources() {
    AudioMixer::add_sound("explosion", AudioMixer::load_sound("data/sounds/explosion.wav"));
    AudioMixer::add_sound("background_music", AudioMixer::load_sound("data/music/background.ogg"));
    // ... TODOS los recursos encontrados
}
```

---

## 🧠 **POR QUÉ ESTA TAREA ES PERFECTA PARA GEMINI**

### Ventajas de 1M Context Window:

1. **Análisis holístico**: Puedes leer TODO el proyecto de una vez
2. **Consistency**: Ves todos los patrones y puedes hacer transformaciones consistentes
3. **Context awareness**: Entiendes si un sonido necesita audio 3D o no según el contexto  
4. **Cross-references**: Puedes seguir llamadas entre archivos
5. **Pattern recognition**: Identificas variantes sutiles de llamadas de audio

### Lo que una IA con context limitado NO puede hacer:

- Ver todo el proyecto simultaneously → transformaciones inconsistentes  
- Entender el contexto completo → decisiones incorrectas sobre 3D vs 2D
- Seguir references entre archivos → pierde llamadas
- Detectar todos los edge cases → migración incompleta

---

## 📝 **DELIVERABLES ESPERADOS**

### 1. `AUDIO_MIGRATION_ANALYSIS.md`
- Lista completa de TODAS las llamadas de audio
- Clasificación por tipo (3D vs 2D)
- Mapeo de archivos de sonido
- Estadísticas (cuántas llamadas por archivo, etc.)

### 2. `AUDIO_MIGRATION_CODE.md`  
- Código de reemplazo para CADA llamada encontrada
- Explicaciones del contexto
- Código de inicialización de recursos

### 3. `AUDIO_MIGRATION_SCRIPT.py` (opcional)
- Script que automatice los reemplazos
- Con validación y backups
- Testing de que no se rompan compilaciones

### 4. `TESTING_CHECKLIST.md`
- Lista de todos los sonidos que deben probarse
- Escenarios de testing (explosiones, UI, etc.)
- Verificación de que audio 3D funciona correctamente

---

## 🚨 **IMPORTANTE: CONTEXTO DE AudioMixer**

Nuestro sistema AudioMixer ya está implementado en:
- `src/AudioMixer.h`
- `src/AudioMixer.cpp`

**API disponible:**
```cpp
class AudioMixer {
public:
    // 2D sound (UI, música)
    static bool play_sound(const std::string& name);
    
    // 3D positional sound (explosiones, pasos, etc.)
    static bool play_sound_3d(const std::string& name, const AudioPosition& pos, float max_distance = 800.0f);
    
    // Resource management
    static MixerAudio* load_sound(const std::string& path);
    static void add_sound(const std::string& name, MixerAudio* audio);
    
    // 3D audio settings
    static void set_listener_position(const AudioPosition& pos);
};

struct AudioPosition {
    float x, y, z;
    AudioPosition(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f);
};
```

---

## 🎯 **RESULTADO FINAL ESPERADO**

Después de tu trabajo, el codebase debería:

✅ **Cero** llamadas a SDL2 Mixer legacy  
✅ **Todas** las explosiones/efectos con audio 3D posicional  
✅ **Todos** los UI sounds usando audio 2D simple  
✅ **Recursos de audio** correctamente inicializados  
✅ **Código compilable** y funcional  
✅ **Performance** mejorado (nuestro AudioMixer es más eficiente)

---

## 💡 **BONUS TASKS** (si tienes tiempo)

1. **Análisis de performance**: Cuánta CPU/memoria se ahorra con la migración
2. **Audio balancing**: Sugerencias de volúmenes y distancias para cada sonido
3. **Missing sounds**: Sonidos que deberían existir pero no están implementados
4. **Audio bugs**: Posibles problemas de audio en el código actual

---

**¡Esta tarea aprovecha perfectamente tu 1M context window para hacer algo que sería súper tedioso para una IA normal! 🚀**