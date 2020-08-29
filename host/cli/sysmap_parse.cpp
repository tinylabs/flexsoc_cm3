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


static PluginTarget *target;

// trim from both ends (in place)
static inline void trim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

int sysmap_parse (const char *map, BusPeripheral ***bp, int *cnt)
{
  int rv;
  
  // Make sure bp and cnt are valid
  if (!bp || !cnt)
    return -1;

  // Create plugin target
  target = new PluginTarget ();
  
  // Clear count
  *cnt = 0;
  
  // Open map file
  ifstream file (map);
  string line;
  
  // Print out map header
  cout << "Virtual Map" << endl << setfill ('-') << setw (11) << "-" << endl << setfill (' ');

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
      cout << "Invalid line [" << line << "]" << endl;
      return -1;
    }

    // Analyze parts
    if ((tokens.size () < 2) || (tokens.size () > 4)) {
      cout << "Invalid line [" << line << "]" << endl;
      return -1;
    }

    // Look for plugin
    obj = plugin_load (plugin.c_str (), tokens[1].c_str(), &type);
    if (!obj) {
      cout << "Plugin not found: " << plugin << endl;
      return -1;
    }

    // Handle bus peripherals
    if (type == BUSPERIPH) {
    
      // Cast to busperipheral
      BusPeripheral *p = (BusPeripheral *)obj;
      
      // Set base address
      p->SetBase (strtoul (addr.c_str (), NULL, 0));

      // Set target operations
      p->SetTarget (target);

      // Initialize bus peripheral
      p->Init ();
      
      // Add to peripheral array
      *bp = (BusPeripheral **)realloc (*bp, sizeof (BusPeripheral *) * (*cnt + 1));
      if (!*bp)
        return -1;
      (*bp)[(*cnt)++] = p;

      // Print out
      cout << left << setbase (16) << uppercase <<
        "  " << p->Base() << ": " << setw (15) << plugin;
      cout << setw (20) << tokens[1] << endl;
    }
    // Handle non busperipheral types
    else {

    }
  }

  // Newline
  //cout << endl;
  
  // Close file
  file.close ();  
  return 0;
}

void sysmap_cleanup (void)
{
  delete target;
}
