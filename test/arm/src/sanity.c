/**
 *  Basic environment sanity check. Make sure variables are initialized
 *  and bss is zeroed.
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */

#include <stdint.h>
#include <string.h>

uint32_t var = 0xaabbccdd;
uint32_t bss[2];

int main (void)
{
  if (var != 0xaabbccdd)
    return -1;

  if ((bss[0] != 0) || (bss[1] != 0))
    return -1;

  // Success
  return 0;
}

