/**
 *  Common test lib c functions
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <stdint.h>
#include "common.h"

uint32_t rand32 (uint32_t *state)
{
  uint32_t tmp;

  if (state == NULL)
    return 0;
  tmp = max32 (*state);
  *state = max32 (tmp);
  return (tmp << 16) | (*state & 0xffff);
}
