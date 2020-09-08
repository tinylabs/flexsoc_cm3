/**
 *  Test plugin memory from ARM
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include "common.h"

// Data
static uint32_t data[16];

static int byte_access (void)
{
  int i;
  uint8_t *ptr = (uint8_t *)0x10000000;
  uint8_t *bdat = (uint8_t *)data;

  // Write out
  for (i = 0; i < sizeof (data); i++)
    ptr[i] = bdat[i];

  // Read back
  for (i = 0; i < sizeof (data); i++)
    if (ptr[i] != bdat[i])
      return -1;
  return 0;
}

static int hwrd_access (void)
{
  int i;
  uint16_t *ptr = (uint16_t *)(0x10000000 + sizeof (data));
  uint16_t *hdat = (uint16_t *)data;

  // Write out
  for (i = 0; i < sizeof (data) / 2; i++)
    ptr[i] = hdat[i];

  // Read back
  for (i = 0; i < sizeof (data) / 2; i++)
    if (ptr[i] != hdat[i])
      return -1;
  return 0;
}

static int word_access (void)
{
  int i;
  uint32_t *ptr = (uint32_t *)(0x10000000 + (2 * sizeof (data)));

  // Write out
  for (i = 0; i < sizeof (data) / 4; i++)
    ptr[i] = data[i];

  // Read back
  for (i = 0; i < sizeof (data) / 4; i++)
    if (ptr[i] != data[i])
      return -1;
  return 0;
}

int main (void)
{
  int i;
  uint32_t seed = 0xdeadbeef;
  uint32_t *ptr = (uint32_t *)0x10000000;
  
  // Generate random
  for (i = 0; i < sizeof (data) / 4; i++)
    data[i] = rand32 (i ? &data[i-1] : &seed);
  
  // Test each width
  if (word_access () || hwrd_access () || byte_access ())
    return -1;

  // Success
  return 0;
}
