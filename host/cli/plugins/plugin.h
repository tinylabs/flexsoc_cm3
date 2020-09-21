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

// Plugin interface
#define PLUGIN(typ, cls, ver)                       \
  OBJ_WRAPPER(cls)                                  \
  extern "C" { extern const plugin_t __plugin; }    \
  const plugin_t __plugin = {                       \
    .version = ver,                                 \
    .create = &cls##_create,                        \
    .type = typ,                                    \
  }



// Helper macros
// Register structure must be nameed reg_t
//
#include <stddef.h>
typedef volatile uint32_t REG32;
typedef volatile uint16_t REG16;
typedef volatile uint8_t  REG8;
#define REG_ADDR(x)  offsetof (reg_t, x)
#define REG_UPDATE(x, data, mask)  reg.x &= ~mask; reg.x |= data & mask;
#define CONCAT_IMPL( x, y ) x##y
#define CONCAT( x, y )      CONCAT_IMPL( x, y )
#define REG_PAD(a1, a2) uint8_t CONCAT(pad, __COUNTER__)[a2-a1]

#endif /* PLUGIN_H */

