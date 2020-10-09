/**
 *  API to connect to remote target via SWD/JTAG bridge
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <stdint.h>

// Open connection to remote
int remote_open (uint8_t div, bool halt);
  

// Close connection to remote
void remote_close (void);

// Dump remote registers
// Remote range 0xE000.0000 must be mapped to base
void remote_dump (uint32_t base);
