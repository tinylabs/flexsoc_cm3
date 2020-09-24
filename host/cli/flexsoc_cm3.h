/**
 *  flexsoc_cm3 header file - Args to call flexsoc_cm3
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#ifndef FLEXSOC_CM3
#define FLEXSOC_CM3

#include <stdint.h>

typedef struct {
  char    *name;
  uint32_t addr;
} load_t;

typedef struct {
  char    *device;     // Device to connect to
  load_t  *load;       // List of files to load
  int     load_cnt;    // Number of files to load
  char    **path;      // Plugin path dirs
  int     path_cnt;    // Number of plugin path
  char    *map;        // System map file
  int     verbose;     // 0=off 3=max
  bool    gdb;         // Do not release reset
  bool    remote;      // Enable remote interface
  uint8_t remote_div;  // Clock div for remote
  bool    remote_halt; // Halt remote processor
} args_t;

int flexsoc_cm3 (args_t *args);

#endif /* FLEXSOC_CM3 */
