/**
 * flexsoc_cm3 main application
 *
 * All rights reserved.
 * Tiny Labs Inc
 * 2020
 */

#include "flexsoc_cm3.h"
#include "err.h"
#include "plugini.h"

#include "Target.h"
#include "flexsoc.h"
#include "log.h"

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <endian.h>

static int shutdown_flag = 0;

// Handle shutdown - this could be caught from ^C or a system unit test completed
static void shutdown (int sig)
{
  signal (SIGINT, SIG_DFL);
  log (LOG_NORMAL, "Shutting down...");
  shutdown_flag = 1;  
}

static uint32_t *read_bin (const char *filename, long *size)
{
  FILE *fp;
  long nsize, bread = 0;
  uint32_t *buf;
  int rv, i;
  
  // Open file
  fp = fopen (filename, "rb");
  if (!fp)
    err ("Failed to open: %s", filename);

  // Get file size
  fseek (fp, 0, SEEK_END);
  *size = ftell (fp);
  fseek (fp, 0, 0);
  if (*size & 3)
    nsize = *size + 4 - (*size & 3);
  else
    nsize = *size;
  
  // Malloc space
  buf = (uint32_t *)malloc (nsize);
  if (!buf)
    err ("Failed to malloc");
  memset (buf, 0, nsize);
  
  // Read into buffer
  while (bread < *size) {
    rv = fread (&buf[bread], 1, *size - bread, fp);
    if (rv <= 0)
      err ("IO error: %d", rv);
    bread += rv;
  }

  // Return nsize
  *size = nsize;
  
  // Close file and return
  fclose (fp);
  return buf;
}

int flexsoc_cm3 (args_t *args)
{
  Target *target;
  uint32_t devid, memid;
  char *device;
  int i;

  // Init logging
  log_init (args->verbose);

  // Copy device as it's modified inplace if simulator
  device = (char *)malloc (strlen (args->device + 1));
  strcpy (device, args->device);
  
  // Find hardware
  target = Target::Ptr (device);

  // Read device_id
  devid = target->FlexsocID ();
  if ((devid >> 4) == 0xF1ec50c)
    log_nonl (LOG_NORMAL, "flexsoc v%d ", devid & 0xf);
  else
    err ("flexsoc not found!");
  
  // Get memory config
  memid = target->MemoryID ();
  log (LOG_NORMAL, "%dkB ROM/%dkB RAM [HW=%s]", memid & 0xffff, (memid >> 16), args->device);
  free (device);
  
  // Load binaries into memory
  for (i = 0; i < args->load_cnt; i++) {
    uint32_t *data, *verify;
    long size;
    
    // Read binary
    log_nonl (LOG_NORMAL, "Loading %s @ 0x%08X... ", args->load[i].name, args->load[i].addr);
    data = read_bin (args->load[i].name, &size);

    // Write to target
    target->WriteW (args->load[i].addr, data, size/4);

    // Malloc buffer to verify
    verify = (uint32_t *)malloc (size);

    // Readback and compare
    target->ReadW (args->load[i].addr, verify, size/4);
    if (memcmp (data, verify, size))
      err ("FAIL");
    log (LOG_NORMAL, "OK");

    // Free buffers
    free (verify);
    free (data);
  }

  // Setup plugins
  if (args->map)
    plugin_init (target, args->map, args->path, args->path_cnt);
  
  // Setup remote link

  // Register handler for shutdown
  signal (SIGINT, &shutdown);
  
  // Release processor from reset
  target->CPUReset (false);

  // Launch TCP server for master access
  while (!shutdown_flag)
    ;
  
  // Put the CPU back into reset
  target->CPUReset (true);

  // Clean up plugins
  plugin_cleanup ();
  
  // Close device
  delete target;

  // Success
  return flexsoc_read_returnval();
}
