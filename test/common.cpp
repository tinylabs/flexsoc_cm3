/**
 *  Common test functions
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <stdint.h>
#include <stddef.h>

uint32_t lfsr (uint32_t val, uint32_t poly, uint8_t *data, size_t len)
{
  int i;

  /* Convert to bits if data is NULL */
  if (data)
    len *= 8;

  /* Loop */
  for (i = 0; i < len; i++) {
    if (((i & 7) == 0) && data)
      val ^= data[i >> 3];
    if (val & 1) {
      val >>= 1;
      val ^= poly;
    }
    else
      val >>= 1;
  }

  return val;
}

uint32_t rand32 (uint32_t *state)
{
  uint32_t tmp;

  if (state==NULL) return 0;
  tmp = lfsr(*state, 0xE0000200, NULL, 32);
  *state = lfsr(tmp, 0xE0000200, NULL, 32);
  return (tmp << 16) | (*state & 0xffff);
}
