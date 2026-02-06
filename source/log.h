/*
 * Logging module - Leveled logging with subscriber pattern
 */

#ifndef LOG_H
#define LOG_H

#include <stdbool.h>

// Log levels (in order of verbosity, most verbose first)
typedef enum {
    LOG_TRACE, // Full request/response bodies, very verbose
    LOG_DEBUG, // Detailed execution information
    LOG_INFO,  // Normal operational messages
    LOG_WARN,  // Warning conditions
    LOG_ERROR, // Error conditions (recoverable)
    LOG_FATAL  // Fatal errors (will exit)
} LogLevel;

// Subscriber callback - receives level and formatted message
typedef void (*LogSubscriber)(LogLevel level, const char *message);

// Maximum number of subscribers
#define LOG_MAX_SUBSCRIBERS 4

// Initialize logging system
void log_init(void);

// Cleanup logging system
void log_exit(void);

// Set minimum log level (messages below this level are ignored)
void log_set_level(LogLevel level);

// Get current log level
LogLevel log_get_level(void);

// Add a subscriber to receive log messages
// Returns true on success, false if max subscribers reached
bool log_subscribe(LogSubscriber subscriber);

// Remove a subscriber
void log_unsubscribe(LogSubscriber subscriber);

// Log at specific levels (printf-style formatting)
void log_trace(const char *fmt, ...);
void log_debug(const char *fmt, ...);
void log_info(const char *fmt, ...);
void log_warn(const char *fmt, ...);
void log_error(const char *fmt, ...);
void log_fatal(const char *fmt, ...);

// Get string name for a log level
const char *log_level_name(LogLevel level);

#endif // LOG_H
