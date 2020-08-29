/**
 *  Generic memory peripheral - Creates arbitrary sized RW memory region on bus
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2019
 */

#include <stdlib.h>
#include <stdio.h>

#include "BusPeripheral.h"
#include "plugin.h"

class Memory : public BusPeripheral {
 private:
  uint32_t *store = NULL;
  
 public:
  Memory (const char *args);
  ~Memory ();
  uint32_t ReadW (uint32_t addr);
  void WriteW (uint32_t addr, uint32_t data, uint32_t mask);
};

// Export memory plugin
PLUGIN (BUSPERIPH, Memory, "v0.0.1");

Memory::Memory (const char *args)
  : BusPeripheral (args)
{
  // Parse size from args
  if (plugin_parse_uint (args, "sz=", &size))
    size = 0;

  // Malloc backing store
  store = (uint32_t *)malloc (size);
  if (!store)
    return;

  // Load memory if specified
  plugin_load_binary (args, "load=", (char *)store, size);
}

Memory::~Memory ()
{
  free (store);
}

uint32_t Memory::ReadW (uint32_t addr)
{
  if (!store)
    return -1;
  return store[addr >> 2];
}

void Memory::WriteW (uint32_t addr, uint32_t data, uint32_t mask)
{
  if (!store)
    return;
  store[addr >> 2] &= ~mask;
  store[addr >> 2] |= (data & mask);
}
