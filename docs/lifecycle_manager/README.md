# LifecycleManager Documentation

Este directorio contiene la documentación completa del sistema LifecycleManager de ClanBomber SDL3.

## Archivos de Documentación

### Documentación Principal

- **[`../LifecycleManager_Overview.md`](../LifecycleManager_Overview.md)** - Visión general del sistema, problemas resueltos y beneficios
- **[`../LifecycleManager_Technical.md`](../LifecycleManager_Technical.md)** - Documentación técnica detallada, API y arquitectura interna

### Ejemplos y Guías

- **[`usage_examples.md`](usage_examples.md)** - Ejemplos prácticos de uso del LifecycleManager
- **[`migration_guide.md`](migration_guide.md)** - Guía para migrar código existente al LifecycleManager
- **[`debugging_guide.md`](debugging_guide.md)** - Técnicas de debugging y troubleshooting

### Código de Implementación

Los archivos principales del LifecycleManager se encuentran en:

- **`src/LifecycleManager.h`** - Definición de la clase y API pública
- **`src/LifecycleManager.cpp`** - Implementación completa del sistema

## Resumen Rápido

El LifecycleManager es un sistema unificado que:

1. **Elimina black gaps** durante la destrucción de objetos
2. **Previene crashes** por double-deletion
3. **Centraliza la gestión** de ciclos de vida de objetos
4. **Simplifica el debugging** con logs centralizados

### Estados del Ciclo de Vida

```
ACTIVE → DYING → DEAD → DELETED
```

- **ACTIVE**: Operación normal
- **DYING**: Animación de destrucción (0.5s para tiles, 0.1s para objetos)
- **DEAD**: Listo para limpieza
- **DELETED**: Removido del juego

### Integración Básica

```cpp
// Registro automático en constructores
GameObject::GameObject(...) {
    if (app && app->lifecycle_manager) {
        app->lifecycle_manager->register_object(this);
    }
}

// Eliminación controlada
objeto->delete_me = true;
// LifecycleManager maneja el resto automáticamente
```

## Contribución

Al modificar el LifecycleManager:

1. **Actualizar documentación** relevante
2. **Agregar tests** para nuevas funcionalidades
3. **Mantener compatibilidad** con código existente
4. **Seguir patrones** establecidos en el diseño

## Problemas Conocidos

- **Performance**: Búsquedas O(n) en listas grandes (optimización futura)
- **Thread Safety**: No implementada aún (pendiente para multiplayer)

---

*Para preguntas específicas sobre el LifecycleManager, consultar la documentación técnica o los ejemplos de uso.*