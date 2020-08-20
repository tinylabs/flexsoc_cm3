/**
 *  Test the latency
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */
#include <stdint.h>
#include <string.h>
#include <time.h>

#include "target.h"
#include "common.h"
#include "err.h"

#define SEED  0xdeadbeef
#define COUNT 20
#define ADDR  0x20000000

// One way host <=> target latency
static unsigned long master_latency_ns;
static unsigned long slave_latency_ns;
static timespec slave_stop;
static Target *target;

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
    target->SlaveSend (resp, 1);
  }
  // Readw
  else {
    resp[0] = 0x30;
    memcpy (&resp[1], "\x11\x22\x33\x44", 4);
    target->SlaveSend (resp, 5);
  }
}

static int master_latency (Target *target)
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
  target->WriteW (ADDR, data, COUNT);

  // Measure latency of each read
  for (i = 0; i < COUNT; i++) {
    timespec start, stop, elapsed;
    uint32_t val;
    
    clock_gettime (CLOCK_REALTIME, &start);
    target->ReadW (ADDR + (4 * i), &val, 1);
    clock_gettime (CLOCK_REALTIME, &stop);

    // Verify read was OK
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

static int slave_latency (Target *target)
{
  int i, rv;
  uint32_t val;
  
  // Test slave read latency
  for (i = 0; i < COUNT; i++) {
    timespec start, elapsed;
    
    // Execute read to invalid memory to get slave request
    clock_gettime (CLOCK_REALTIME, &start);
    target->ReadW (0x40000000, &val, 1);

    // Get elapsed, subtract master
    elapsed = diff (start, slave_stop);

    // Average
    if (i)
      slave_latency_ns = (slave_latency_ns + elapsed.tv_nsec) / 2;
    else
      slave_latency_ns = elapsed.tv_nsec;
  }
  slave_latency_ns /= 2;
  printf ("slave latency:  %.2fus\n", (float)slave_latency_ns / 1000);
  
  return 0;
}

int main (int argc, char **argv)
{
  int rv;
  uint32_t val = 3;
  
  if (argc != 2)
    err ("Must pass interface");

  // Open target
  target = new Target (argv[1]);
  
  // Test master latency
  if (master_latency (target))
    err ("Master latency test failed");

  // Register slave
  target->SlaveRegister (&slave_cb);

  // Enable slave interface
  target->SlaveEn (true);
  
  // Test slave latency
  if (slave_latency (target))
    err ("Slave latency test failed");

  // Close target
  delete target;
  return 0;
}
