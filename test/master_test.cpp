/**
 *  Test the master interface of flexsoc_cm3
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <stdint.h>
#include <string.h>
#include "flexsoc.h"
#include "common.h"
#include "err.h"

#define SEED 0xdeadbeef
#define ADDR 0x0

static int write_test (void)
{
  int i;
  uint32_t seed = SEED;
  uint32_t dat[4], exp[4];
  uint16_t *dath = (uint16_t *)dat;
  uint8_t *datb = (uint8_t *)dat;
  
  // Generate random data
  for (i = 0; i < 4; i++)
    dat[i] = rand32 (i ? &dat[i-1] : &seed);

  // Write word data
  if (flexsoc_writew (ADDR, dat, 4))
    return -1;

  // Write hwrd data
  if (flexsoc_writeh (ADDR + 0x10, dath, 8))
    return -1;
  
  // Write word data
  if (flexsoc_writeb (ADDR + 0x20, datb, 16))
    return -1;

  return 0;
}

static int read_test (void)
{
  int i;
  uint32_t seed = SEED;
  uint32_t dat[4], exp[4];
  uint16_t *exph = (uint16_t *)exp;
  uint16_t *dath = (uint16_t *)dat;
  uint8_t *expb = (uint8_t *)exp;
  uint8_t *datb = (uint8_t *)dat;
  
  // Generate random data
  for (i = 0; i < 4; i++)
    exp[i] = rand32 (i ? &exp[i-1] : &seed);

  // Read word data
  memset (dat, 0, sizeof (dat));
  if (flexsoc_readw (ADDR, dat, 4))
    return -1;

  // Verify data
  if (memcmp (exp, dat, sizeof (dat)))
    return -1;

  // Read hwrd data
  memset (dat, 0, sizeof (dat));
  if (flexsoc_readh (ADDR + 0x10, dath, 8))
    return -1;

  // Verify data
  if (memcmp (exph, dath, sizeof (dat)))
    return -1;

  // Read word data
  memset (dat, 0, sizeof (dat));
  if (flexsoc_readb (ADDR + 0x20, datb, 16))
    return -1;

  // Verify data
  if (memcmp (expb, datb, sizeof (dat)))
    return -1;

  return 0;
}

int main (int argc, char **argv)
{
  int rv;

  if (argc != 2)
    err ("Must pass interface");

  // Open interface to flexsoc
  rv = flexsoc_open (argv[1]);
  if (rv)
    err ("Failed to open: %s", argv[1]);

  // Run write tests
  if (write_test ())
    err ("Write test failed");
  
  // Run read tests
  if (read_test ())
    err ("Read test failed");

  // Close interface
  flexsoc_close ();
  return 0;
}
