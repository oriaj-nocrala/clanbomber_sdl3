#pragma once

#include <string>
#include <exception>
#include <functional>
#include <vector>
#include <memory>

/**
 * @brief Tipos de errores del juego organizados por categoría
 */
enum class GameErrorType {
    // === RENDERING ERRORS ===
    RENDER_ERROR,
    GRAPHICS_INIT_FAILED,
    SHADER_COMPILATION_FAILED,
    TEXTURE_LOAD_FAILED,
    OPENGL_ERROR,
    
    // === RESOURCE ERRORS ===
    MAP_LOAD_FAILED,
    AUDIO_INIT_FAILED,
    FONT_LOAD_FAILED,
    CONFIG_LOAD_FAILED,
    
    // === GAME LOGIC ERRORS ===
    OBJECT_CREATION_FAILED,
    INVALID_GAME_STATE,
    COLLISION_DETECTION_ERROR,
    
    // === NETWORK ERRORS ===
    NETWORK_CONNECTION_FAILED,
    SERVER_TIMEOUT,
    CLIENT_DISCONNECTED,
    
    // === SYSTEM ERRORS ===
    OUT_OF_MEMORY,
    FILE_IO_ERROR,
    SDL_ERROR,
    
    // === GENERIC ===
    UNKNOWN_ERROR
};

/**
 * @brief Nivel de severidad del error
 */
enum class ErrorSeverity {
    INFO,       // Solo informativo, el juego continúa
    WARNING,    // Advertencia, posible problema futuro
    ERROR,      // Error, funcionalidad afectada pero juego continúa  
    CRITICAL    // Error crítico, el juego no puede continuar
};

/**
 * @brief Estrategia de recovery para cada tipo de error
 */
enum class ErrorRecoveryStrategy {
    CONTINUE,           // Continuar ignorando el error
    RETRY,             // Intentar la operación de nuevo
    FALLBACK,          // Usar implementación alternativa
    RESTART_SUBSYSTEM, // Reiniciar el subsistema afectado
    GRACEFUL_EXIT      // Salir del juego de forma controlada
};

/**
 * @brief Excepción base del juego con información contextual
 */
class GameException : public std::exception {
public:
    GameException(GameErrorType type, ErrorSeverity severity, 
                  const std::string& message, const std::string& context = "")
        : error_type(type), severity(severity), error_message(message), error_context(context) {}

    const char* what() const noexcept override {
        return error_message.c_str();
    }
    
    GameErrorType get_type() const { return error_type; }
    ErrorSeverity get_severity() const { return severity; }
    const std::string& get_context() const { return error_context; }

private:
    GameErrorType error_type;
    ErrorSeverity severity;
    std::string error_message;
    std::string error_context;
};

/**
 * @brief Result type para operaciones que pueden fallar
 * Similar al Result<T,E> de Rust
 */
template<typename T>
class GameResult {
public:
    // Success constructor
    static GameResult success(T&& value) {
        return GameResult(std::forward<T>(value));
    }
    
    static GameResult success(const T& value) {
        return GameResult(value);
    }
    
    // Error constructor
    static GameResult error(GameErrorType type, ErrorSeverity severity, 
                           const std::string& message, const std::string& context = "") {
        return GameResult(type, severity, message, context);
    }
    
    // Check if operation was successful
    bool is_ok() const { return has_value; }
    bool is_error() const { return !has_value; }
    
    // Get value (throws if error)
    const T& get_value() const {
        if (!has_value) {
            throw GameException(error_type, severity, error_message, error_context);
        }
        return value;
    }
    
    T& get_value() {
        if (!has_value) {
            throw GameException(error_type, severity, error_message, error_context);
        }
        return value;
    }
    
    // Get value with default fallback
    T get_or_default(const T& default_value) const {
        return has_value ? value : default_value;
    }
    
    // Get error info
    GameErrorType get_error_type() const { return error_type; }
    ErrorSeverity get_severity() const { return severity; }
    const std::string& get_error_message() const { return error_message; }
    const std::string& get_error_context() const { return error_context; }

private:
    // Success constructor
    explicit GameResult(T&& val) : has_value(true), value(std::forward<T>(val)) {}
    explicit GameResult(const T& val) : has_value(true), value(val) {}
    
    // Error constructor
    GameResult(GameErrorType type, ErrorSeverity sev, const std::string& msg, const std::string& ctx)
        : has_value(false), error_type(type), severity(sev), error_message(msg), error_context(ctx) {}

    bool has_value = false;
    T value{};
    
    // Error info
    GameErrorType error_type = GameErrorType::UNKNOWN_ERROR;
    ErrorSeverity severity = ErrorSeverity::ERROR;
    std::string error_message;
    std::string error_context;
};

// Specialization for void operations
template<>
class GameResult<void> {
public:
    static GameResult success() {
        return GameResult(true);
    }
    
    static GameResult error(GameErrorType type, ErrorSeverity severity,
                           const std::string& message, const std::string& context = "") {
        return GameResult(type, severity, message, context);
    }
    
    bool is_ok() const { return is_success; }
    bool is_error() const { return !is_success; }
    
    GameErrorType get_error_type() const { return error_type; }
    ErrorSeverity get_severity() const { return severity; }
    const std::string& get_error_message() const { return error_message; }
    const std::string& get_error_context() const { return error_context; }

private:
    explicit GameResult(bool ok) : is_success(ok) {}
    
    GameResult(GameErrorType type, ErrorSeverity sev, const std::string& msg, const std::string& ctx)
        : is_success(false), error_type(type), severity(sev), error_message(msg), error_context(ctx) {}

    bool is_success = false;
    GameErrorType error_type = GameErrorType::UNKNOWN_ERROR;
    ErrorSeverity severity = ErrorSeverity::ERROR;
    std::string error_message;
    std::string error_context;
};

/**
 * @brief Handler de errores centralizado con estrategias de recovery
 */
class ErrorHandler {
public:
    using ErrorCallback = std::function<ErrorRecoveryStrategy(const GameException&)>;
    
    static ErrorHandler& getInstance() {
        static ErrorHandler instance;
        return instance;
    }

    /**
     * @brief Maneja un error según su tipo y severidad
     * @param error La excepción a manejar
     * @return Estrategia de recovery recomendada
     */
    ErrorRecoveryStrategy handle_error(const GameException& error);
    
    /**
     * @brief Registra un callback personalizado para un tipo de error
     * @param error_type Tipo de error
     * @param callback Función callback
     */
    void register_error_handler(GameErrorType error_type, ErrorCallback callback);
    
    /**
     * @brief Registra todos los errores para debugging
     * @param enabled true para habilitar logging
     */
    void set_error_logging(bool enabled) { log_errors = enabled; }
    
    /**
     * @brief Obtiene estadísticas de errores
     */
    struct ErrorStats {
        size_t total_errors = 0;
        size_t critical_errors = 0;
        size_t warnings = 0;
        std::vector<std::pair<GameErrorType, size_t>> error_counts;
    };
    
    ErrorStats get_error_statistics() const;
    
    /**
     * @brief Limpia estadísticas de errores
     */
    void clear_error_statistics();

private:
    ErrorHandler() = default;
    
    std::vector<std::pair<GameErrorType, ErrorCallback>> error_handlers;
    bool log_errors = true;
    
    // Statistics
    mutable std::vector<std::pair<GameErrorType, size_t>> error_counts;
    mutable size_t total_error_count = 0;
    
    void log_error(const GameException& error);
    ErrorRecoveryStrategy get_default_strategy(GameErrorType type, ErrorSeverity severity);
};

/**
 * @brief Macros para facilitar el uso del sistema de errores
 */
#define RETURN_ERROR(type, severity, message, context) \
    return GameResult<decltype(type)>::error(type, severity, message, context)

#define RETURN_SUCCESS(value) \
    return GameResult<decltype(value)>::success(value)

#define HANDLE_ERROR_OR_RETURN(result, recovery_action) \
    if ((result).is_error()) { \
        auto exception = GameException((result).get_error_type(), (result).get_severity(), \
                                     (result).get_error_message(), (result).get_error_context()); \
        auto strategy = ErrorHandler::getInstance().handle_error(exception); \
        if (strategy == ErrorRecoveryStrategy::recovery_action) { \
            /* Execute recovery action */ \
        } else { \
            return result; \
        } \
    }
