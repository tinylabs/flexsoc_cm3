/**
 *  Load plugins - return plugin object after loading
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2019,2020
 */
#ifndef PLUGIN_H
#define PLUGIN_H

#include "pluginhelper.h"

// Signature to create objects
// this must be the same for all plugins
typedef void *(*plugin_create) (const char *);

typedef enum {
  BUSPERIPH = 1,
  UARTPERIPH,
  SPIPERIPH,
  I2CPERIPH,
  GPIOPERIPH,
  TYPE_MAX_ENUM,
} plugin_type_t;

// Exported plugin structure
typedef struct {
  const char   *version;
  plugin_create create;
  plugin_type_t type;
} plugin_t;

#define OBJ_WRAPPER(cls)                                                \
  extern "C" {                                                          \
    void *cls##_create (const char *args) {                             \
      return new cls (args);                                            \
    }                                                                   \
  }

/* Plugin interface */
#define PLUGIN(typ, cls, ver)                       \
  OBJ_WRAPPER(cls)                                  \
  extern "C" { extern const plugin_t __plugin; }    \
  const plugin_t __plugin = {                       \
    .version = ver,                                 \
    .create = &cls##_create,                        \
    .type = typ,                                    \
  }

#endif /* PLUGIN_H */

