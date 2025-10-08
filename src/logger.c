#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>

static FILE *log_file_handle = NULL;

int init_logger(const char *log_file) {
    log_file_handle = fopen(log_file, "a");
    if (!log_file_handle) {
        fprintf(stderr, "Failed to open log file: %s\n", log_file);
        return -1;
    }
    return 0;
}

void close_logger(void) {
    if (log_file_handle) {
        log_file_handle = NULL;
    }
}

static const char *level_to_string(LogLevel level) {
    switch (level) {
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARNING: return "WARNING";
        case LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

void log_message(LogLevel level, const char *format, ...) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    va_list args;
    va_start(args, format);

    // Log to file
    if (log_file_handle) {
        fprintf(log_file_handle, "[%s] [%s] ", timestamp, level_to_string(level));
        vprintf(log_file_handle, format, args);
        fprintf(log_file_handle, "\n");
        fflush(log_file_handle);
    }

    if (level >= LOG_WARNING) {
        va_end(args);
        va_start(args, format);
        fprintf(stderr, "[%s] [%s] ", timestamp, level_to_string(level));
        vprintf(stderr, format, args);
        fprintf(stderr, "\n");
    }

    va_end(args);
}

void log_debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_DEBUG, format, args);
    va_end(args);
}

void log_info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_INFO, format, args);
    va_end(args);
}

void log_warning(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_WARNING, format, args);
    va_end(args);
}

void log_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    log_message(LOG_ERROR, format, args);
    va_end(args);
}