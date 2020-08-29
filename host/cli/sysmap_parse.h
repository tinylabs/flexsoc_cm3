/**
 *  Parse sysmap containing plugin placement and arguments.
 *  This subsystem then instantiates those plugins and connects
 *  them as specified.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#ifndef SYSMAP_PARSE_H
#define SYSMAP_PARSE_H

#include "BusPeripheral.h"

// Pass in system map
// Returns pointer to list of bus peripherals and bus peripheral count
int sysmap_parse (const char *map, BusPeripheral ***bp, int *cnt);

// Don't call unless shutting down system
void sysmap_cleanup (void);

#endif /* SYSMAP_PARSE_H */

