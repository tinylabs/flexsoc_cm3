/**
 *  This function parses the system map and instantiates and connects the necessary plugins.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2019
 */

#include <iostream>
#include <fstream>
#include <algorithm>

#include <stdlib.h>
#include <bits/stdc++.h>

#include "sysmap_parse.h"
#include "plugini.h"
#include "plugin.h"

#include "Target.h"
#include "PluginTarget.h"
#include "log.h"

/**
 *  The system maps is specified in the following format:
 *   - declaration per line
 *   - # for comment
 *
 *  Each declaration consists of either:
 *    plugin@address:args:export (bus peripheral)
 *  or:
 *    plugin@export1,export2:args (secondary peripheral)
 *  or:
 *    rirq@A:B (Map remote IRQ B to local IRQ A) A/B=[0,240)
 *  or:
 *    rirq@A-B:C-D (Map remote IRQ range C-D to local range A-B)
 *
 *  For instance a GPIO controller and SPI controller may be mapped as:
 *    pl061@0x40001000::A   (maps ARM primecell GPIO periph to addr, export as port A)
 *    pl022@0x40002000::SPI.0  (maps ARM primecell SPI periph to addr, export as SPI.0)
 *
 *  Note: Exports must match a valid interface. The following are valid interfaces:
 *    UART
 *    I2C
 *    SPI
 *    A-G - GPIO interface
 *
 *  A simulated OLED can then be connected to the SPI bus:
 *    ssd1306@SPI.0,A.9:rotate=1
 *
 *  The SSD1306 OLED controller is now connected to the SPI.0 bus. Pin A.9 (port A, pin 9)
 *  terminates to the OLED controller as a CS pin. This allows other peripherals to
 *  connect to the SPI.0 bus. ie:
 *    sd@SPI.0,A.10:sz=4G
 *
 *  The beauty of all of this is (if done right) the firmware running on the target
 *  is 100% compatible between physical hardware and virtual peripherals. This makes
 *  it easy to quickly prototype a new system using mostly virtual peripherals and then
 *  migrate them over one-by-one until you have a fully physical system.
 *
 *  In addition to the previous examples there are other "peripherals" with no
 *  physical analog. These can be very useful for debugging/reverse engineering.
 *  For instance you may have a binary image that runs on a piece of hardware that
 *  you're trying to understand. By connecting the JTAG/SWD bridge to the physical
 *  hardware and mapping that port (not covered here) you can perform a sort of
 *  man-in-the-middle to log all peripheral accesses.
 *
 *  Assuming the target is configured to map the local JTAG/SWD bridge 
 *  @0x6000.0000 -> remote 0x4000.0000 we can add the following line to the system
 *  map:
 *    remap@0x40000000:@0x60000000 sz=256M log=periph.txt
 *   
 *  This will map all peripheral accesses from the original firmware to the JTAG/SWD
 *  bridge which will then complete on the original hardware. In addition accesses
 *  will be logged a file for later analysis. Of course the downside is bus accesses
 *  will get stretched when taking this path such that "clock time" and "system time"
 *  will drift.
 */

using namespace std;


static PluginTarget *ptarget;

// trim from both ends (in place)
static inline void trim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static uint32_t parse_uint (const char *str)
{
  char *end;
  uint32_t val = strtoul (str, &end, 0);
  if (*end == 'k')
    val *= 1024;
  else if (*end == 'M')
    val *= (1024 * 1024);
  return val;
}

int sysmap_parse (const char *map, BusPeripheral ***bp, int *cnt)
{
  int rv = 0;
  
  // Make sure bp and cnt are valid
  if (!bp || !cnt)
    return -1;

  // Get host target
  Target *target = Target::Ptr ();
  
  // Create plugin target
  ptarget = new PluginTarget ();
  
  // Clear count
  *cnt = 0;
  
  // Open map file
  ifstream file (map);
  string line;
  
  // Print out map header
  log (LOG_NORMAL, "Plugin Map:");

  // Loop over each line
  while (getline (file, line)) {
    void *obj;
    plugin_type_t type;
    size_t pos;
    string plugin, addr;
    
    // Trim whitespace
    trim (line);
    
    // Check if line starts with #
    if ((line[0] == '#') || (line == ""))
      continue;

    // Tokenize into parts
    vector <string> tokens;
    stringstream decl (line);
    string token;

    // Loop over tokens
    while (getline (decl, token, ':'))
      tokens.push_back (token);

    // Split plugin and address
    if ((pos = tokens[0].find ("@")) != string::npos) {

      // Get plugin name
      plugin = tokens[0].substr (0, pos);

      // Get address
      addr = tokens[0].substr (pos + 1, tokens[0].length ());
    }
    else {
      log (LOG_ERR, "Invalid line [%s]", line);
      rv = -1;
      goto cleanup;
    }

    // Analyze parts
    if ((tokens.size () < 1) || (tokens.size () > 4)) {
      log (LOG_ERR, "Invalid line [%s]", line);
      rv = -1;
      goto cleanup;
    }

    // Handle builtin functions
    if (plugin == "alias") {
      alias_t alias;
      uint8_t idx = 0;
      
      if (tokens.size () != 3) {
        log (LOG_ERR, "Invalid alias: alias@base:size:redirect");
        rv = -1;
        goto cleanup;
      }
      
      // Configure memory alias
      if (parse_uint (addr.c_str ()) < 0x20000000) {
        if (target->CodeAliasGet (idx, &alias))
          log (LOG_ERR, "Failed to get code alias[%d]", idx);

        // Check if alias used
        if (alias.base != 0)
          idx++;
      }
      
      // Setup alias
      alias.base = parse_uint (addr.c_str ());
      alias.size = parse_uint (tokens[1].c_str ());
      alias.remap = parse_uint (tokens[2].c_str ());

      // Configure registers
      if (alias.base < 0x20000000) {
        if (target->CodeAliasSet (idx, &alias))
          log (LOG_ERR, "Failed to set code alias %d", idx);
      }
      else {
        if (target->SysAliasSet (idx, &alias))
          log (LOG_ERR, "Failed to set code alias %d", idx);
      }
    }
    else if (plugin == "remap32") {
      if ((tokens.size () != 2) || (stoi (addr) < 0) || (stoi (addr) > 7)) {
        log (LOG_ERR, "Invalid remap32: remap32@<0-7>:<remote addr>");
        rv = -1;
        goto cleanup;
      }

      // Configure remote mapping
      target->RemoteRemap32M (parse_uint (addr.c_str()), parse_uint (tokens[1].c_str()));
    }
    else if (plugin == "remap256") {
      if (tokens.size () != 1) {
        log (LOG_ERR, "Invalid remap256: remap32@<remote addr>");
        rv = -1;
        goto cleanup;
      }

      // Configure remote mapping
      target->RemoteRemap256M (parse_uint (addr.c_str ()));
    }
    else {
      
      // Look for plugin
      if (tokens.size () > 1)
        obj = plugin_load (plugin.c_str (), tokens[1].c_str(), &type);
      else
        obj = plugin_load (plugin.c_str (), "", &type);

      if (!obj) {
        log (LOG_ERR, "Plugin not found: %s", plugin);
        rv = -1;
        goto cleanup;
      }

      // Handle bus peripherals
      if (type == BUSPERIPH) {
        
        // Cast to busperipheral
        BusPeripheral *p = (BusPeripheral *)obj;
        
        // Set base address
        p->SetBase (strtoul (addr.c_str (), NULL, 0));
        
        // Set target operations
        p->SetTarget (ptarget);
        
        // Initialize bus peripheral
        p->Init ();
        
        // Add to peripheral array
        *bp = (BusPeripheral **)realloc (*bp, sizeof (BusPeripheral *) * (*cnt + 1));
        if (!*bp) {
          rv = -1;
          goto cleanup;
        }
        (*bp)[(*cnt)++] = p;
        
        // Print out
        log (LOG_NORMAL, "  %08X: %s", p->Base(), plugin.c_str());
        if (tokens.size() > 1)
          log (LOG_DEBUG, "  %s", tokens[1].c_str());
      }
      // Handle non busperipheral types
      else {
        
      }
    }
  }

  cleanup:
    
    // Close file
    file.close ();
    
    if (rv)
      delete ptarget;
    
    return rv;
}

void sysmap_cleanup (void)
{
  delete ptarget;
}
