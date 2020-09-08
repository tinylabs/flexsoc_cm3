/**
 *  flexsoc logging
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include "log.h"
#include <stdio.h>

static uint8_t log_level = LOG_NORMAL;

void log_init (uint8_t lvl)
{
  log_level = lvl;
}

void vlog (uint8_t lvl, const char *fmt, va_list ap)
{
  if (lvl <= log_level)
    vprintf (fmt, ap);
}

void log (uint8_t lvl, const char *fmt, ...)
{
  va_list va;
  va_start (va, fmt);
  vlog (lvl, fmt, va);
  if (lvl <= log_level)
    printf ("\n");
  va_end (va);
}

void log_nonl (uint8_t lvl, const char *fmt, ...)
{
  va_list va;
  va_start (va, fmt);
  vlog (lvl, fmt, va);
  va_end (va);
}

