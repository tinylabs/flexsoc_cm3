/**
 *  Test the master interface of flexsoc_cm3
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "flexsoc.h"
#include "common.h"
#include "err.h"

#define SEED       0xdeadbeef
#define BUFSZ_WORD (8*1024)/4
//#define BUFSZ_WORD (1*1024)/4
#define ADDR       0x00000000

static timespec diff(timespec start, timespec end)
{
	timespec temp;
	if ((end.tv_nsec-start.tv_nsec)<0) {
		temp.tv_sec = end.tv_sec-start.tv_sec-1;
		temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
	} else {
		temp.tv_sec = end.tv_sec-start.tv_sec;
		temp.tv_nsec = end.tv_nsec-start.tv_nsec;
	}
	return temp;
}

static double tsFloat (timespec  time)
{
  return ((double) time.tv_sec + (time.tv_nsec / 1000000000.0));
}

static int write_throughput (void)
{
  int i, rv;
  uint32_t seed = SEED;
  uint32_t *data;
  timespec start, stop, elapsed;
  
  // Malloc buffer
  data = (uint32_t *)malloc (BUFSZ_WORD * sizeof (uint32_t));
  if (!data)
    err ("Failed to malloc buf");
  
  // Generate random data
  for (i = 0; i < BUFSZ_WORD; i++)
    data[i] = rand32 (i ? &data[i-1] : &seed);

  // Write to RAM
  clock_gettime (CLOCK_REALTIME, &start);
  rv = flexsoc_writew (ADDR, data, BUFSZ_WORD);
  clock_gettime (CLOCK_REALTIME, &stop);

  // Calculate time
  elapsed = diff (start, stop);
  
  // Release buffer
  free (data);

  // Print write throughput
  printf ("write_throughput:  %lu bytes/sec\n",
          ((long)((BUFSZ_WORD * sizeof (uint32_t))/tsFloat (elapsed))));
  return 0;
}

static int read_throughput (void)
{
  int i, rv;
  uint32_t seed = SEED;
  uint32_t *exp, *data;
  timespec start, stop, elapsed;
  
  // Malloc buffer
  data = (uint32_t *)malloc (BUFSZ_WORD * sizeof (uint32_t));
  exp = (uint32_t *)malloc (BUFSZ_WORD * sizeof (uint32_t));
  if (!exp || !data)
    err ("Failed to malloc buf");
  
  // Generate random data
  for (i = 0; i < BUFSZ_WORD; i++)
    exp[i] = rand32 (i ? &exp[i-1] : &seed);

  // Write to RAM
  clock_gettime (CLOCK_REALTIME, &start);
  rv = flexsoc_readw (ADDR, data, BUFSZ_WORD);
  clock_gettime (CLOCK_REALTIME, &stop);

  // Verify data
  for (i = 0; i < BUFSZ_WORD; i++)
    if (exp[i] != data[i])
      err ("[%d] Read mismatch: %08X != %08X", i, exp[i], data[i]);
  
  // Calculate time
  elapsed = diff (start, stop);
  
  // Release buffer 
  free (exp);
  free (data);
  
  // Print write throughput
  printf ("read_throughput:  %lu bytes/sec\n",
          ((long)((BUFSZ_WORD * sizeof (uint32_t))/tsFloat (elapsed))));
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

  // Test master latency
  if (write_throughput ())
    err ("Throughput test failed");
  
  if (read_throughput ())
    err ("Throughput test failed");
  
  // Close interface
  flexsoc_close ();
  return 0;
}
