/**
 *  flexsoc_cm3 header file - Args to call flexsoc_cm3
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#ifndef FLEXSOC_CM3
#define FLEXSOC_CM3

typedef struct {
  char **load;     // List of files to load
  int    load_cnt; // Number of files to load
  int    verbose;  // 0=off 3=max
} args_t;

int flexsoc_cm3 (args_t *args);

#endif /* FLEXSOC_CM3 */
