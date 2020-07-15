/**
 *  Test the master interface of flexsoc_cm3
 *
 *  All rights reserved.
 *  Tiny Labs Inc
 *  2020
 */

#include "flexsoc.h"
#include "err.h"

static int write_test (void)
{
  return 0;
}

static int read_test (void)
{
  return 0;
}

int main (int argc, char **argv)
{
  int rv;

  if (argc != 2)
    err ("Must pass interface");
  
  // Open interface to flexsoc
  rv = flexsoc_open (argv[1], NULL);
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
