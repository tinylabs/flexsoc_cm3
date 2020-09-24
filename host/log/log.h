/**
 *  Flexsoc logging library
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */

#ifndef LOG_H
#define LOG_H

#include <stdint.h>
#include <stdarg.h>

#define LOG_FATAL  -2  // Exit if fatal
#define LOG_ERR    -1  // Always print errors
#define LOG_SILENT  0  // Mostly silent (apart from errors)
#define LOG_NORMAL  1  // Normal printouts
#define LOG_DEBUG   2  // Add debug info
#define LOG_TRACE   3  // Trace at transaction level
#define LOG_TRANS   4  // Trace transport layer

// Init logging
void log_init (int8_t log_level);

// Log message
void log (int8_t lvl, const char *fmt, ...);
void log_nonl (int8_t lvl, const char *fmt, ...);
void vlog (int8_t lvl, const char *fmt, va_list ap);

#endif /* LOG_H */
