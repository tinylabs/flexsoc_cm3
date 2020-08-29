/**
 *  Simple hook to exit the simulator gracefully with an error code when 
 *  written to a special location.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2019
 */

#include <stdlib.h>
#include <stdio.h>

#include "BusPeripheral.h"
#include "plugin.h"

class UnitTest : public BusPeripheral {
  
 public:
  UnitTest (const char *args);
  ~UnitTest () {}
  uint32_t ReadW (uint32_t addr);
  void WriteW (uint32_t addr, uint32_t data, uint32_t mask);
};

// Export plugin
PLUGIN (BUSPERIPH, UnitTest, "v0.0.1");

UnitTest::UnitTest (const char *args)
  : BusPeripheral (args)
{
  // Save size
  size = 4;
}

uint32_t UnitTest::ReadW (uint32_t addr)
{
  return 1;
}

void UnitTest::WriteW (uint32_t addr, uint32_t data, uint32_t mask)
{
  // Return success if data is 0x20026
  target->Exit (data == 0x20026 ? 0 : -1);
}
