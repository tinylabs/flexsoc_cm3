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

#define SEED  0xdeadbeef
#define COUNT 100
#define ADDR  0x20000000

// One way host <=> target latency
static unsigned long master_latency_ns;
static unsigned long slave_latency_ns;
static timespec slave_stop;

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

static void slave_cb (uint8_t *buf, int len)
{
  uint8_t resp[5];
  clock_gettime (CLOCK_REALTIME, &slave_stop);

  // Send dummy responses
  // Write
  if (buf[0] & 0x08) {
    resp[0] = 0x00;
    flexsoc_send (resp, 1);
  }
  // Readw
  else {
    resp[0] = 0x30;
    memcpy (&resp[1], "\x11\x22\x33\x44", 4);
    flexsoc_send (resp, 5);
  }
}

static int master_latency (void)
{
  int i, rv;
  uint32_t seed = SEED;
  uint32_t *data;
  
  // Malloc buffer
  data = (uint32_t *)malloc (COUNT * sizeof (uint32_t));
  if (!data)
    err ("Failed to malloc buf");
  
  // Generate random data
  for (i = 0; i < COUNT; i++)
    data[i] = rand32 (i ? &data[i-1] : &seed);

  // Write to RAM
  if (flexsoc_writew (ADDR, data, COUNT))
    return -1;

  // Measure latency of each read
  for (i = 0; i < COUNT; i++) {
    timespec start, stop, elapsed;
    uint32_t val;
    
    clock_gettime (CLOCK_REALTIME, &start);
    rv = flexsoc_readw (ADDR + (4 * i), &val, 1);
    clock_gettime (CLOCK_REALTIME, &stop);

    // Verify read was OK
    if (rv)
      err ("Read failed: %d", rv);
    if (val != data[i])
      err ("[%d] %08X != %08X", i, val, data[i]);

    // Calculate time
    elapsed = diff (start, stop);
    if (i)
      master_latency_ns = (master_latency_ns + elapsed.tv_nsec) / 2;
    else
      master_latency_ns = elapsed.tv_nsec;
  }

  // Release buffer
  free (data);

  // Divide by two to get rough latency host->target
  master_latency_ns /= 2;
  printf ("master latency: %.2fus\n", (float)master_latency_ns / 1000);
  return 0;
}

static int slave_latency (void)
{
  int i, rv;
  uint32_t val;
  
  // Test slave read latency
  for (i = 0; i < COUNT; i++) {
    timespec start, elapsed;
    
    // Execute read to invalid memory to get slave request
    clock_gettime (CLOCK_REALTIME, &start);
    rv = flexsoc_readw (0x40000000, &val, 1);

    // Get elapsed, subtract master
    elapsed = diff (start, slave_stop);
    if (master_latency_ns < elapsed.tv_nsec)
      elapsed.tv_nsec -= master_latency_ns;

    // Average
    if (i)
      slave_latency_ns = (slave_latency_ns + elapsed.tv_nsec) / 2;
    else
      slave_latency_ns = elapsed.tv_nsec;
  }
  printf ("slave latency:  %.2fus\n", (float)slave_latency_ns / 1000);
  
  return 0;
}

int main (int argc, char **argv)
{
  int rv;
  uint32_t val = 3;
  
  if (argc != 2)
    err ("Must pass interface");

  // Open interface to flexsoc
  rv = flexsoc_open (argv[1], &slave_cb);
  if (rv)
    err ("Failed to open: %s", argv[1]);

  // Test master latency
  if (master_latency ())
    err ("Master latency test failed");

  // Enable slave interface
  if (flexsoc_writew (0xE0000004, &val, 1))
    err ("Failed to enable slave interface");
  
  // Test slave latency
  if (slave_latency ())
    err ("Slave latency test failed");

  // Close interface
  flexsoc_close ();
  return 0;
}
