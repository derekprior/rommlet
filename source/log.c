/*
 * Logging module - Leveled logging with subscriber pattern
 */

#include "log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define LOG_BUFFER_SIZE 1024

static LogLevel currentLevel = LOG_INFO;
static LogSubscriber subscribers[LOG_MAX_SUBSCRIBERS] = {NULL};
static int subscriberCount = 0;

static const char *levelNames[] = {
    "TRACE",
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL"
};

void log_init(void) {
    currentLevel = LOG_INFO;
    subscriberCount = 0;
    for (int i = 0; i < LOG_MAX_SUBSCRIBERS; i++) {
        subscribers[i] = NULL;
    }
}

void log_exit(void) {
    subscriberCount = 0;
    for (int i = 0; i < LOG_MAX_SUBSCRIBERS; i++) {
        subscribers[i] = NULL;
    }
}

void log_set_level(LogLevel level) {
    currentLevel = level;
}

LogLevel log_get_level(void) {
    return currentLevel;
}

bool log_subscribe(LogSubscriber subscriber) {
    if (!subscriber) return false;
    
    // Check if already subscribed
    for (int i = 0; i < subscriberCount; i++) {
        if (subscribers[i] == subscriber) {
            return true;
        }
    }
    
    if (subscriberCount >= LOG_MAX_SUBSCRIBERS) {
        return false;
    }
    
    subscribers[subscriberCount++] = subscriber;
    return true;
}

void log_unsubscribe(LogSubscriber subscriber) {
    for (int i = 0; i < subscriberCount; i++) {
        if (subscribers[i] == subscriber) {
            // Shift remaining subscribers down
            for (int j = i; j < subscriberCount - 1; j++) {
                subscribers[j] = subscribers[j + 1];
            }
            subscribers[--subscriberCount] = NULL;
            return;
        }
    }
}

const char *log_level_name(LogLevel level) {
    if (level >= 0 && level <= LOG_FATAL) {
        return levelNames[level];
    }
    return "UNKNOWN";
}

static void log_message(LogLevel level, const char *fmt, va_list args) {
    // Skip if below current log level
    if (level < currentLevel) {
        return;
    }
    
    // Format the message
    char buffer[LOG_BUFFER_SIZE];
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    
    // Notify all subscribers
    for (int i = 0; i < subscriberCount; i++) {
        if (subscribers[i]) {
            subscribers[i](level, buffer);
        }
    }
}

void log_trace(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_TRACE, fmt, args);
    va_end(args);
}

void log_debug(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_DEBUG, fmt, args);
    va_end(args);
}

void log_info(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_INFO, fmt, args);
    va_end(args);
}

void log_warn(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_WARN, fmt, args);
    va_end(args);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_ERROR, fmt, args);
    va_end(args);
}

void log_fatal(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    log_message(LOG_FATAL, fmt, args);
    va_end(args);
}
