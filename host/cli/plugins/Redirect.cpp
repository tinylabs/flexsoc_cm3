/**
 *  Redirect (and log) all accesses
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2019
 */

#include <stdlib.h>
#include <stdio.h>

#include "BusPeripheral.h"
#include "plugin.h"

class Redirect : public BusPeripheral {
 private:
  uint32_t base;
  uint32_t log_lvl;
  
 public:
  Redirect (const char *args);
  ~Redirect () {}
  uint32_t ReadW (uint32_t addr);
  void WriteW (uint32_t addr, uint32_t data, uint32_t mask);
};

// Export redirect plugin
PLUGIN (BUSPERIPH, Redirect, "v0.0.1");

Redirect::Redirect (const char *args)
  : BusPeripheral (args)
{
  // Parse size from args
  if (plugin_parse_uint (args, "sz=", &size))
    size = 0;

  // Parse size from args
  if (plugin_parse_uint (args, "@", &base))
    base = 0;

  // Check if we're logging (1=enable/0=disable)
  if (plugin_parse_uint (args, "log=", &log_lvl))
    log_lvl = 0;
  log_lvl = log_lvl ? 1 : 5;
}

uint32_t Redirect::ReadW (uint32_t addr)
{
  uint32_t val;
  val = target->ReadW (base + addr);
  target->Log (log_lvl, "R %08X=%08X\n", base + addr, val);
  return val;
}

void Redirect::WriteW (uint32_t addr, uint32_t data, uint32_t mask)
{
  target->WriteW (base + addr, data, mask);
  target->Log (log_lvl, "W %08X=%08X [%08X]\n", base + addr, data, mask);
}
