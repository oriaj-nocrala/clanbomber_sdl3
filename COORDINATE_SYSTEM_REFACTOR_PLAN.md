# 🎯 **PLAN DE REFACTORIZACIÓN MASIVA DEL SISTEMA DE COORDENADAS**

## **OBJETIVO**: Hacer CoordinateSystem la única fuente de verdad

---

## **🚨 PROBLEMA IDENTIFICADO**

La auditoría exhaustiva reveló **50+ violaciones** del principio de responsabilidad única:

### **Violaciones Críticas:**
- ✗ **50+ instancias** de cálculos manuales `/40`, `*40`, `+20`
- ✗ **6 métodos legacy** en GameObject.h duplican funcionalidad de CoordinateSystem
- ✗ **Números mágicos** hardcodeados en lugar de `CoordinateConfig::TILE_SIZE`
- ✗ **Inconsistencia**: Algunos objetos usan CoordinateSystem, otros usan cálculos manuales
- ✗ **Violación del principio**: No hay una sola fuente de verdad para coordenadas

---

## **PHASE 1: Reemplazar números mágicos con constantes** ⚡
**Prioridad: CRÍTICA** | **Riesgo: BAJO** | **Impacto: MEDIO**

### Acciones:
1. **GameContext.cpp**: `spatial_grid = new SpatialGrid(CoordinateConfig::TILE_SIZE)`
2. **MapTile_Pure.h**: Usar `CoordinateConfig::TILE_SIZE` en lugar de `40`
3. **Todos los archivos**: Reemplazar `40` → `CoordinateConfig::TILE_SIZE`, `20` → `CoordinateConfig::TILE_SIZE/2`

### Archivos afectados:
- GameContext.cpp (línea 31)
- MapTile_Pure.h (líneas 43-44)
- Map.cpp (línea 149)
- TileManager.cpp (línea 347)

---

## **PHASE 2: Eliminar métodos legacy de GameObject.h** ⚡
**Prioridad: CRÍTICA** | **Riesgo: ALTO** | **Impacto: ALTO**

### Acciones:
1. **Deprecar métodos legacy**:
   - `pixel_to_map_x()`, `pixel_to_map_y()`
   - `map_to_pixel_x()`, `map_to_pixel_y()`
   - `get_center_map_x()`, `get_center_map_y()`

2. **Crear métodos wrapper que usan CoordinateSystem**:
```cpp
int get_center_map_x() const { 
    return CoordinateSystem::pixel_to_grid(PixelCoord(x, y)).grid_x; 
}
int get_center_map_y() const { 
    return CoordinateSystem::pixel_to_grid(PixelCoord(x, y)).grid_y; 
}
```

3. **Eventualmente eliminar métodos legacy completamente**

### ⚠️ Riesgo: Puede romper código dependiente

---

## **PHASE 3: Refactorizar cálculos manuales en GameObject.cpp** ⚡
**Prioridad: CRÍTICA** | **Riesgo: MEDIO** | **Impacto: ALTO**

### Problemas críticos a arreglar:

#### **Función snap()** (líneas 602-603):
```cpp
// ANTES:
x = ((get_x()+20)/40)*40;
y = ((get_y()+20)/40)*40;

// DESPUÉS:
GridCoord grid = CoordinateSystem::pixel_to_grid(PixelCoord(x, y));
PixelCoord center = CoordinateSystem::grid_to_pixel(grid);
x = static_cast<int>(center.pixel_x);
y = static_cast<int>(center.pixel_y);
```

#### **Métodos get_map_x(), get_map_y()** (líneas 663, 677):
```cpp
// ANTES:
int tmp = get_x()/40;

// DESPUÉS:
GridCoord grid = CoordinateSystem::pixel_to_grid(PixelCoord(get_x(), get_y()));
int tmp = grid.grid_x;
```

#### **Métodos de dirección** (líneas 719-734):
```cpp
// ANTES:
return get_tile_type_at(x+20, y+40);

// DESPUÉS:
PixelCoord center(x, y);
PixelCoord below = PixelCoord(center.pixel_x, center.pixel_y + CoordinateConfig::TILE_SIZE);
return get_tile_type_at(static_cast<int>(below.pixel_x), static_cast<int>(below.pixel_y));
```

#### **Métodos get_tile()** (líneas 747, 761, 775):
```cpp
// ANTES:
return map->get_tile( (int)(x+20)/40, (int)(y+20)/40 );

// DESPUÉS:
GridCoord grid = CoordinateSystem::pixel_to_grid(PixelCoord(x, y));
return map->get_tile(grid.grid_x, grid.grid_y);
```

#### **Métodos is_tile_blocking_at(), has_bomb_at(), has_bomber_at()** (líneas 789-850):
```cpp
// ANTES:
int map_x = pixel_x / 40;
int map_y = pixel_y / 40;

// DESPUÉS:
GridCoord grid = CoordinateSystem::pixel_to_grid(PixelCoord(pixel_x, pixel_y));
int map_x = grid.grid_x;
int map_y = grid.grid_y;
```

### Estrategia:
- Una función a la vez para evitar romper funcionalidad
- Compilar y probar después de cada cambio
- Mantener firma de métodos públicos

---

## **PHASE 4: Refactorizar AI Controllers** ⚠️
**Prioridad: ALTA** | **Riesgo: MEDIO** | **Impacto: MEDIO**

### Controller_AI_Smart.cpp - 15+ violaciones:

#### **Movimiento direccional** (líneas 261-263, 276-278):
```cpp
// ANTES:
if (next_step.x > my_pos.x + 20) current_input.right = true;

// DESPUÉS:
PixelCoord my_center = PixelCoord(my_pos.x, my_pos.y);
PixelCoord next_center = PixelCoord(next_step.x, next_step.y);
if (next_center.pixel_x > my_center.pixel_x + CoordinateConfig::TILE_SIZE/2) current_input.right = true;
```

#### **Conversiones de coordenadas** (líneas 362-363, 375-376, 492-496):
```cpp
// ANTES:
int grid_x = (int)(next_pos.x / 40);
int grid_y = (int)(next_pos.y / 40);

// DESPUÉS:
GridCoord grid = CoordinateSystem::pixel_to_grid(PixelCoord(next_pos.x, next_pos.y));
int grid_x = grid.grid_x;
int grid_y = grid.grid_y;
```

#### **Cálculo de área de explosión** (líneas 713-716):
```cpp
// ANTES:
tiles.push_back(vector_add(bomb_pos, CL_Vector(i * 40, 0)));

// DESPUÉS:
GridCoord bomb_grid = CoordinateSystem::pixel_to_grid(PixelCoord(bomb_pos.x, bomb_pos.y));
GridCoord target_grid(bomb_grid.grid_x + i, bomb_grid.grid_y);
PixelCoord target_pixel = CoordinateSystem::grid_to_pixel(target_grid);
tiles.push_back(CL_Vector(target_pixel.pixel_x, target_pixel.pixel_y));
```

### Controller_AI_Modern.cpp (línea 145):
```cpp
// ANTES:
if (bomber->has_bomb_at(bomber->get_x() + 20, bomber->get_y() + 20)) {

// DESPUÉS:
PixelCoord bomber_pos(bomber->get_x(), bomber->get_y());
GridCoord bomber_grid = CoordinateSystem::pixel_to_grid(bomber_pos);
PixelCoord tile_center = CoordinateSystem::grid_to_pixel(bomber_grid);
if (bomber->has_bomb_at(static_cast<int>(tile_center.pixel_x), static_cast<int>(tile_center.pixel_y))) {
```

---

## **PHASE 5: Otros archivos críticos** ⚠️
**Prioridad: MEDIA** | **Riesgo: BAJO** | **Impacto: MEDIO**

### Archivos a refactorizar:

#### **TileEntity.cpp** (línea 49):
```cpp
// ANTES:
SDL_Log("CRITICAL: TileEntity::act() - tile_data corrupted at (%d,%d)!", get_x()/40, get_y()/40);

// DESPUÉS:
GridCoord grid = CoordinateSystem::pixel_to_grid(PixelCoord(get_x(), get_y()));
SDL_Log("CRITICAL: TileEntity::act() - tile_data corrupted at (%d,%d)!", grid.grid_x, grid.grid_y);
```

#### **MapTile.cpp** (línea 40):
```cpp
// ANTES:
context->get_lifecycle_manager()->register_tile(tile, x/40, y/40);

// DESPUÉS:
GridCoord grid = CoordinateSystem::pixel_to_grid(PixelCoord(x, y));
context->get_lifecycle_manager()->register_tile(tile, grid.grid_x, grid.grid_y);
```

#### **Map.cpp** (línea 149):
```cpp
// ANTES:
x*40, y*40, context);

// DESPUÉS:
GridCoord grid(x, y);
PixelCoord center = CoordinateSystem::grid_to_pixel(grid);
static_cast<int>(center.pixel_x), static_cast<int>(center.pixel_y), context);
```

#### **TileManager.cpp** (línea 347):
```cpp
// ANTES:
map_x * 40, map_y * 40, context

// DESPUÉS:
GridCoord grid(map_x, map_y);
PixelCoord center = CoordinateSystem::grid_to_pixel(grid);
static_cast<int>(center.pixel_x), static_cast<int>(center.pixel_y), context
```

---

## **PHASE 6: Verificación final** ✅
**Prioridad: CRÍTICA** | **Riesgo: BAJO** | **Impacto: CRÍTICO**

### Verificaciones automatizadas:
```bash
# 1. Buscar violaciones restantes
grep -r "\* 40\|/ 40\|+ 20" src/ --include="*.cpp" --include="*.h"

# 2. Verificar uso correcto de CoordinateSystem
grep -r "CoordinateSystem::" src/ --include="*.cpp" 

# 3. Buscar números mágicos
grep -r "\b40\b|\b20\b" src/ --include="*.cpp" --include="*.h" | grep -v CoordinateConfig

# 4. Verificar que no hay cálculos manuales
grep -r "x.*[*/].*40\|y.*[*/].*40" src/ --include="*.cpp" --include="*.h"
```

### Pruebas funcionales:
1. **Bombers spawn correctamente** - Verificar posición centrada en tiles
2. **Bombs se colocan centradas** - Sin desalineamiento visual
3. **Extras aparecen alineados** - Centrados en tiles destruidos
4. **Colisiones tile-perfect** - Solo cuando están en mismo tile exacto
5. **AI funciona correctamente** - Sin errores de pathfinding
6. **Escape system funciona** - Bomber puede salir de bomba propia

---

## **🚨 ORDEN DE EJECUCIÓN RECOMENDADO**

### **CRÍTICO - Empezar YA:**
1. ✅ **Phase 1** - Números mágicos (bajo riesgo, fácil)
2. ✅ **Phase 3** - GameObject.cpp (crítico, necesario)

### **IMPORTANTE - Después:**
3. ✅ **Phase 2** - GameObject.h legacy methods  
4. ✅ **Phase 4** - AI Controllers

### **LIMPIEZA - Al final:**
5. ✅ **Phase 5** - Otros archivos
6. ✅ **Phase 6** - Verificación final

---

## **💡 ESTRATEGIA DE IMPLEMENTACIÓN**

### **Incremental y segura:**
- ✅ Un archivo a la vez
- ✅ Compilar después de cada cambio
- ✅ Probar funcionalidad después de cada change
- ✅ Mantener funcionalidad existente durante transición
- ✅ Usar wrapper methods como puente
- ✅ Commit después de cada phase exitosa

### **Principios a seguir:**
1. **DRY**: No repetir cálculos de coordenadas
2. **SRP**: CoordinateSystem es la única fuente de verdad
3. **Consistency**: Todas las conversiones usan la misma lógica
4. **Maintainability**: Cambios futuros solo en CoordinateSystem
5. **Testability**: Fácil de verificar y probar

### **Rollback plan:**
- Git branch para cada phase
- Commits granulares
- Ability to revert individual changes
- Mantener backwards compatibility durante transición

---

## **📊 MÉTRICAS DE ÉXITO**

### **Pre-refactoring:**
- ❌ 50+ cálculos manuales
- ❌ 6 métodos duplicados
- ❌ 10+ números mágicos
- ❌ Inconsistencia en coordinatas

### **Post-refactoring:**
- ✅ 0 cálculos manuales
- ✅ 0 métodos duplicados  
- ✅ 0 números mágicos
- ✅ 100% consistencia via CoordinateSystem

---

## **⚡ COMENZAR IMPLEMENTACIÓN**

**Comando para empezar:**
```bash
git checkout -b coordinate-system-refactor
```

**Estado actual: READY TO BEGIN PHASE 1**