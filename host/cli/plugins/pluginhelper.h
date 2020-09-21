/**
 *  Plugin helper routines to assist with parsing, etc
 *
 *  All rights reserved.
 *  Tiny Labs Inc.
 *  2019
 */
#ifndef PLUGINHELPER_H
#define PLUGINHELPER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

  // Parse args string and return val
  int plugin_parse_uint (const char *args, const char *key, uint32_t *val);
  int plugin_parse_bool (const char *args, const char *key, bool *val);
  int plugin_parse_str (const char *args, const char *key, const char **str, int *len);
  
  // Load binary from file into ptr
  int plugin_load_binary (const char *args, const char *key, char *ptr, int max);
  
#ifdef __cplusplus
}
#endif

#endif /* PLUGINHELPER_H */

