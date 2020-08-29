/**
 *  Plugin interface - Host plugins allow the emulation of bus peripherals on a 
 *  host system. These plugins are mapped to the host system when a matching 
 *  transaction on the AHB interconnect is found.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#ifndef PLUGINI_H
#define PLUGINI_H

#include <stdint.h>
#include "plugin.h"
#include "Target.h"

// Init plugin interface - pass search path
void plugin_init (Target *target, char *sysmap, char **path, int cnt);

// Load plugin
void *plugin_load (const char *name, const char *args, plugin_type_t *type);

// Cleanup plugins
void plugin_cleanup (void);

#endif /* PLUGINI_H */

