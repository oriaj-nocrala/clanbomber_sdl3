#include "ErrorHandling.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <iostream>

ErrorRecoveryStrategy ErrorHandler::handle_error(const GameException& error) {
    // Log error if enabled
    if (log_errors) {
        log_error(error);
    }
    
    // Update statistics
    total_error_count++;
    auto it = std::find_if(error_counts.begin(), error_counts.end(),
        [&error](const auto& pair) { return pair.first == error.get_type(); });
    
    if (it != error_counts.end()) {
        it->second++;
    } else {
        error_counts.emplace_back(error.get_type(), 1);
    }
    
    // Check for custom handlers
    for (const auto& handler : error_handlers) {
        if (handler.first == error.get_type()) {
            return handler.second(error);
        }
    }
    
    // Use default strategy based on error type and severity
    return get_default_strategy(error.get_type(), error.get_severity());
}

void ErrorHandler::register_error_handler(GameErrorType error_type, ErrorCallback callback) {
    // Remove existing handler for this type if any
    error_handlers.erase(
        std::remove_if(error_handlers.begin(), error_handlers.end(),
            [error_type](const auto& pair) { return pair.first == error_type; }),
        error_handlers.end());
    
    // Add new handler
    error_handlers.emplace_back(error_type, callback);
}

ErrorHandler::ErrorStats ErrorHandler::get_error_statistics() const {
    ErrorStats stats;
    stats.total_errors = total_error_count;
    stats.error_counts = error_counts;
    
    // Calculate critical errors and warnings (this is simplified)
    for (const auto& pair : error_counts) {
        // This is a rough estimation - in a real implementation you'd track severity per error
        switch (pair.first) {
            case GameErrorType::GRAPHICS_INIT_FAILED:
            case GameErrorType::OUT_OF_MEMORY:
            case GameErrorType::SHADER_COMPILATION_FAILED:
                stats.critical_errors += pair.second;
                break;
            case GameErrorType::TEXTURE_LOAD_FAILED:
            case GameErrorType::AUDIO_INIT_FAILED:
            case GameErrorType::FONT_LOAD_FAILED:
                stats.warnings += pair.second;
                break;
            default:
                // Other errors
                break;
        }
    }
    
    return stats;
}

void ErrorHandler::clear_error_statistics() {
    error_counts.clear();
    total_error_count = 0;
}

void ErrorHandler::log_error(const GameException& error) {
    const char* severity_str;
    switch (error.get_severity()) {
        case ErrorSeverity::INFO:     severity_str = "INFO"; break;
        case ErrorSeverity::WARNING:  severity_str = "WARNING"; break;
        case ErrorSeverity::ERROR:    severity_str = "ERROR"; break;
        case ErrorSeverity::CRITICAL: severity_str = "CRITICAL"; break;
        default: severity_str = "UNKNOWN"; break;
    }
    
    const char* type_str;
    switch (error.get_type()) {
        case GameErrorType::GRAPHICS_INIT_FAILED:        type_str = "GRAPHICS_INIT_FAILED"; break;
        case GameErrorType::SHADER_COMPILATION_FAILED:   type_str = "SHADER_COMPILATION_FAILED"; break;
        case GameErrorType::TEXTURE_LOAD_FAILED:         type_str = "TEXTURE_LOAD_FAILED"; break;
        case GameErrorType::OPENGL_ERROR:                type_str = "OPENGL_ERROR"; break;
        case GameErrorType::MAP_LOAD_FAILED:             type_str = "MAP_LOAD_FAILED"; break;
        case GameErrorType::AUDIO_INIT_FAILED:           type_str = "AUDIO_INIT_FAILED"; break;
        case GameErrorType::FONT_LOAD_FAILED:            type_str = "FONT_LOAD_FAILED"; break;
        case GameErrorType::CONFIG_LOAD_FAILED:          type_str = "CONFIG_LOAD_FAILED"; break;
        case GameErrorType::OBJECT_CREATION_FAILED:      type_str = "OBJECT_CREATION_FAILED"; break;
        case GameErrorType::INVALID_GAME_STATE:          type_str = "INVALID_GAME_STATE"; break;
        case GameErrorType::COLLISION_DETECTION_ERROR:   type_str = "COLLISION_DETECTION_ERROR"; break;
        case GameErrorType::NETWORK_CONNECTION_FAILED:   type_str = "NETWORK_CONNECTION_FAILED"; break;
        case GameErrorType::SERVER_TIMEOUT:              type_str = "SERVER_TIMEOUT"; break;
        case GameErrorType::CLIENT_DISCONNECTED:         type_str = "CLIENT_DISCONNECTED"; break;
        case GameErrorType::OUT_OF_MEMORY:               type_str = "OUT_OF_MEMORY"; break;
        case GameErrorType::FILE_IO_ERROR:               type_str = "FILE_IO_ERROR"; break;
        case GameErrorType::SDL_ERROR:                   type_str = "SDL_ERROR"; break;
        case GameErrorType::UNKNOWN_ERROR:               type_str = "UNKNOWN_ERROR"; break;
        default: type_str = "UNDEFINED"; break;
    }
    
    if (error.get_context().empty()) {
        SDL_Log("GameError [%s] %s: %s", severity_str, type_str, error.what());
    } else {
        SDL_Log("GameError [%s] %s: %s (Context: %s)", 
                severity_str, type_str, error.what(), error.get_context().c_str());
    }
}

ErrorRecoveryStrategy ErrorHandler::get_default_strategy(GameErrorType type, ErrorSeverity severity) {
    // Critical errors usually require graceful exit
    if (severity == ErrorSeverity::CRITICAL) {
        return ErrorRecoveryStrategy::GRACEFUL_EXIT;
    }
    
    // Default strategies based on error type
    switch (type) {
        // Graphics errors - try fallback first, then restart subsystem
        case GameErrorType::GRAPHICS_INIT_FAILED:
        case GameErrorType::SHADER_COMPILATION_FAILED:
            return ErrorRecoveryStrategy::RESTART_SUBSYSTEM;
            
        case GameErrorType::TEXTURE_LOAD_FAILED:
        case GameErrorType::FONT_LOAD_FAILED:
            return ErrorRecoveryStrategy::FALLBACK;
            
        // Resource loading - retry once, then fallback
        case GameErrorType::MAP_LOAD_FAILED:
        case GameErrorType::CONFIG_LOAD_FAILED:
            return ErrorRecoveryStrategy::RETRY;
            
        // Audio can usually be disabled safely
        case GameErrorType::AUDIO_INIT_FAILED:
            return ErrorRecoveryStrategy::FALLBACK;
            
        // Game logic errors - continue with caution
        case GameErrorType::OBJECT_CREATION_FAILED:
        case GameErrorType::COLLISION_DETECTION_ERROR:
            return ErrorRecoveryStrategy::CONTINUE;
            
        // Network errors - retry for temporary issues
        case GameErrorType::NETWORK_CONNECTION_FAILED:
        case GameErrorType::SERVER_TIMEOUT:
            return ErrorRecoveryStrategy::RETRY;
            
        case GameErrorType::CLIENT_DISCONNECTED:
            return ErrorRecoveryStrategy::CONTINUE;
            
        // Memory errors are usually critical
        case GameErrorType::OUT_OF_MEMORY:
            return ErrorRecoveryStrategy::GRACEFUL_EXIT;
            
        // SDL and file errors - retry or continue
        case GameErrorType::SDL_ERROR:
        case GameErrorType::FILE_IO_ERROR:
            return ErrorRecoveryStrategy::RETRY;
            
        default:
            return ErrorRecoveryStrategy::CONTINUE;
    }
}